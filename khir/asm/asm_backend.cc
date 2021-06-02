#include "khir/asm/asm_backend.h"

#include <iostream>
#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "khir/instruction.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

using namespace asmjit;

void ExceptionErrorHandler::handleError(Error err, const char* message,
                                        BaseEmitter* origin) {
  throw std::runtime_error(message);
}

uint64_t ASMBackend::OutputConstant(
    uint64_t instr, const TypeManager& type_manager,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals) {
  auto opcode = GenericInstructionReader(instr).Opcode();

  switch (opcode) {
    case Opcode::I1_CONST:
    case Opcode::I8_CONST: {
      // treat I1 as I8
      uint8_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt8(constant);
      return 1;
    }

    case Opcode::I16_CONST: {
      uint16_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt16(constant);
      return 2;
    }

    case Opcode::I32_CONST: {
      uint32_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt32(constant);
      return 4;
    }

    case Opcode::I64_CONST: {
      auto i64_id = Type1InstructionReader(instr).Constant();
      asm_->embedUInt64(i64_constants[i64_id]);
      return 8;
    }

    case Opcode::F64_CONST: {
      auto f64_id = Type1InstructionReader(instr).Constant();
      asm_->embedDouble(f64_constants[f64_id]);
      return 8;
    }

    case Opcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto str_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(char_array_constants_[str_id]);
      return 8;
    }

    case Opcode::NULLPTR: {
      asm_->embedUInt64(0);
      return 8;
    }

    case Opcode::GLOBAL_REF: {
      auto global_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(globals_[global_id]);
      return 8;
    }

    case Opcode::STRUCT_CONST: {
      auto struct_id = Type1InstructionReader(instr).Constant();
      const auto& struct_const = struct_constants[struct_id];

      auto field_offsets =
          type_manager.GetStructFieldOffsets(struct_const.Type());
      auto type_size = type_manager.GetTypeSize(struct_const.Type());
      auto field_constants = struct_const.Fields();

      uint64_t bytes_written = 0;

      for (int i = 0; i < field_constants.size(); i++) {
        auto field_const = field_constants[i];

        // add any padding until we're at the offset
        while (bytes_written < field_offsets[i]) {
          asm_->embedUInt8(0);
          bytes_written++;
        }

        bytes_written += OutputConstant(
            field_const, type_manager, i64_constants, f64_constants,
            char_array_constants, struct_constants, array_constants, globals);
      }

      while (bytes_written < type_size) {
        asm_->embedUInt8(0);
        bytes_written++;
      }

      return bytes_written;
    }

    case Opcode::ARRAY_CONST: {
      auto arr_id = Type1InstructionReader(instr).Constant();
      const auto& arr_const = array_constants[arr_id];

      uint64_t bytes_written = 0;
      for (auto elem_const : arr_const.Elements()) {
        // arrays have no padding
        bytes_written += OutputConstant(
            elem_const, type_manager, i64_constants, f64_constants,
            char_array_constants, struct_constants, array_constants, globals);
      }
      return bytes_written;
    }

    default:
      throw std::runtime_error("Invalid constant.");
  }
}

void ASMBackend::Translate(const TypeManager& type_manager,
                           const std::vector<uint64_t>& i64_constants,
                           const std::vector<double>& f64_constants,
                           const std::vector<std::string>& char_array_constants,
                           const std::vector<StructConstant>& struct_constants,
                           const std::vector<ArrayConstant>& array_constants,
                           const std::vector<Global>& globals,
                           const std::vector<Function>& functions) {
  code_.init(rt_.environment());
  asm_ = std::make_unique<x86::Assembler>(&code_);

  Section* text_section = code_.textSection();
  Section* data_section;
  code_.newSection(&data_section, ".data", SIZE_MAX, 0, 8, 0);

  asm_->section(data_section);

  // Write out all string constants
  for (const auto& str : char_array_constants) {
    char_array_constants_.push_back(asm_->newLabel());
    asm_->bind(char_array_constants_.back());
    for (char c : str) {
      asm_->embedUInt8(static_cast<uint8_t>(c));
    }
    asm_->embedUInt8(0);
  }

  // Write out all global variables
  for (const auto& global : globals) {
    globals_.push_back(asm_->newLabel());
    asm_->bind(globals_.back());
    OutputConstant(global.InitialValue(), type_manager, i64_constants,
                   f64_constants, char_array_constants, struct_constants,
                   array_constants, globals);
  }

  asm_->section(text_section);

  // Declare all functions
  for (const auto& function : functions) {
    external_func_addr_.push_back(function.Addr());
    internal_func_labels_.push_back(asm_->newLabel());
  }

  const std::vector<x86::Gpq> callee_saved_registers = {
      x86::rbx, x86::r12, x86::r13, x86::r14, x86::r15};

  // Translate all function bodies
  for (int func_idx = 0; func_idx < functions.size(); func_idx++) {
    const auto& func = functions[func_idx];
    if (func.External()) {
      continue;
    }

    num_floating_point_args_ = 0;
    num_regular_args_ = 0;
    num_stack_args_ = 0;

    asm_->bind(internal_func_labels_[func_idx]);

    auto bpoint = asm_->newLabel();
    breakpoints_[func.Name()] = bpoint;
    asm_->bind(bpoint);

    if (func.Name() == "compute") {
      compute_label_ = internal_func_labels_[func_idx];
    }

    // Prologue ================================================================
    // - Save RBP and Store RSP in RBP
    asm_->push(x86::rbp);
    asm_->mov(x86::rbp, x86::rsp);

    // - Save all callee saved registers
    for (auto reg : callee_saved_registers) {
      asm_->push(reg);
    }

    // - Allocate placeholder stack space.
    auto stack_alloc_offset = asm_->offset();
    asm_->long_().sub(x86::rsp, 0);
    // =========================================================================

    const auto& instructions = func.Instructions();
    const auto& basic_blocks = func.BasicBlocks();
    const auto& basic_block_order = func.BasicBlockOrder();

    std::vector<Label> basic_blocks_impl;
    Label epilogue = asm_->newLabel();
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(asm_->newLabel());
    }

    std::vector<int64_t> value_offsets(instructions.size(), INT64_MAX);

    // initially, after rbp is all callee saved registers
    int64_t static_stack_frame_size = 8 * callee_saved_registers.size();
    for (int i : basic_block_order) {
      asm_->bind(basic_blocks_impl[i]);
      const auto& [i_start, i_end] = basic_blocks[i];
      for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
        TranslateInstr(type_manager, i64_constants, f64_constants,
                       basic_blocks_impl, functions, epilogue, value_offsets,
                       instructions, instr_idx, static_stack_frame_size);
      }
    }

    // we need to ensure that it is aligned to 16 bytes so if we call any
    // function, stack is 16 byte aligned
    static_stack_frame_size += (16 - (static_stack_frame_size % 16)) % 16;
    assert(static_stack_frame_size % 16 == 0);

    // Update prologue to contain stack_size
    // Already pushed callee saved registers so don't double allocate space
    int64_t stack_alloc_size =
        static_stack_frame_size - 8 * callee_saved_registers.size();
    auto epilogue_offset = asm_->offset();
    asm_->setOffset(stack_alloc_offset);
    asm_->long_().sub(x86::rsp, stack_alloc_size);
    asm_->setOffset(epilogue_offset);

    // Epilogue ================================================================
    // - Dealloc stack space
    asm_->bind(epilogue);
    asm_->mov(x86::rax, x86::rbp);
    asm_->sub(x86::rax, 8 * callee_saved_registers.size());
    asm_->mov(x86::rsp, x86::rax);

    // - Restore callee saved registers
    for (int i = callee_saved_registers.size() - 1; i >= 0; i--) {
      asm_->pop(callee_saved_registers[i]);
    }

    // - Restore RBP
    asm_->pop(x86::rbp);

    // - Return
    asm_->ret();
    // =========================================================================
  }
}

void ASMBackend::ComparisonInRdx(Opcode op) {
  switch (op) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ:
    case Opcode::F64_CMP_EQ: {
      asm_->sete(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE:
    case Opcode::F64_CMP_NE: {
      asm_->setne(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT:
    case Opcode::F64_CMP_LT: {
      asm_->setl(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE:
    case Opcode::F64_CMP_LE: {
      asm_->setle(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT:
    case Opcode::F64_CMP_GT: {
      asm_->setg(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_CMP_GE: {
      asm_->setge(x86::dil);
      asm_->movzx(x86::rdx, x86::dil);
      return;
    }

    default:
      throw std::runtime_error("Invalid comparison.");
  }
}

void ASMBackend::TranslateInstr(const TypeManager& type_manager,
                                const std::vector<uint64_t>& i64_constants,
                                const std::vector<double>& f64_constants,
                                const std::vector<Label>& basic_blocks,
                                const std::vector<Function>& functions,
                                const Label& epilogue,
                                std::vector<int64_t>& offsets,
                                const std::vector<uint64_t>& instructions,
                                int instr_idx, int64_t& static_stack_alloc) {
  auto instr = instructions[instr_idx];
  auto opcode = GenericInstructionReader(instr).Opcode();

  switch (opcode) {
    case Opcode::STRUCT_CONST:
    case Opcode::ARRAY_CONST: {
      // should only be used to initialize globals so referencing them in
      // a normal context shouldn't do anything
      return;
    }

    case Opcode::I8_CONST:
    case Opcode::I1_CONST: {
      auto constant = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::rax, constant);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_CONST: {
      auto constant = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::rax, constant);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_CONST: {
      auto constant = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::rax, constant);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::NULLPTR:
    case Opcode::I64_CONST: {
      if (opcode == Opcode::NULLPTR) {
        asm_->mov(x86::rax, 0);
      } else {
        auto i64_id = Type1InstructionReader(instr).Constant();
        asm_->mov(x86::rax, i64_constants[i64_id]);
      }

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_CONST: {
      uint64_t f64_id = Type1InstructionReader(instr).Constant();
      double value = f64_constants[f64_id];

      uint64_t value_as_int;
      std::memcpy(&value_as_int, &value, sizeof(value_as_int));

      asm_->mov(x86::rax, value_as_int);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto char_id = Type1InstructionReader(instr).Constant();
      auto label = char_array_constants_[char_id];

      asm_->lea(x86::rax, x86::ptr(label));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::GLOBAL_REF: {
      auto glob_id = Type1InstructionReader(instr).Constant();
      auto label = globals_[glob_id];

      asm_->lea(x86::rax, x86::ptr(label));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cmp(x86::al, x86::cl);
      ComparisonInRdx(opcode);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cmp(x86::ax, x86::cx);
      ComparisonInRdx(opcode);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cmp(x86::eax, x86::ecx);
      ComparisonInRdx(opcode);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cmp(x86::rax, x86::rcx);
      ComparisonInRdx(opcode);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->ucomisd(x86::xmm0, x86::xmm1);
      ComparisonInRdx(opcode);
      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->add(x86::al, x86::cl);
      asm_->movzx(x86::rdx, x86::al);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_SUB: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->sub(x86::al, x86::cl);
      asm_->movzx(x86::rdx, x86::al);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_MUL: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->imul(x86::cl);
      asm_->movzx(x86::rdx, x86::al);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_DIV: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->idiv(x86::cl);
      asm_->movzx(x86::rdx, x86::al);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->add(x86::ax, x86::cx);
      asm_->movzx(x86::rdx, x86::ax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_SUB: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->sub(x86::ax, x86::cx);
      asm_->movzx(x86::rdx, x86::ax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_MUL: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->imul(x86::cx);
      asm_->movzx(x86::rdx, x86::ax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_DIV: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cwd();
      asm_->idiv(x86::cx);
      asm_->movzx(x86::rdx, x86::ax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->add(x86::eax, x86::ecx);
      asm_->movzx(x86::rdx, x86::eax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_SUB: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->sub(x86::eax, x86::ecx);
      asm_->movzx(x86::rdx, x86::eax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_MUL: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->imul(x86::ecx);
      asm_->movzx(x86::rdx, x86::eax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_DIV: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cdq();
      asm_->idiv(x86::ecx);
      asm_->movzx(x86::rdx, x86::eax);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_ADD:
    case Opcode::PTR_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->add(x86::rax, x86::rcx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->sub(x86::rax, x86::rcx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->imul(x86::rcx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_DIV: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->cqo();
      asm_->idiv(x86::rcx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I1_ZEXT_I8:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->xor_(x86::rax, 1);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->addsd(x86::xmm0, x86::xmm1);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->mulsd(x86::xmm0, x86::xmm1);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->subsd(x86::xmm0, x86::xmm1);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->divsd(x86::xmm0, x86::xmm1);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->cvttsd2si(x86::rax, x86::xmm0);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_CONV_F64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->movsx(x86::rax, x86::al);
      asm_->cvtsi2sd(x86::xmm0, x86::rax);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_CONV_F64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->movsx(x86::rax, x86::ax);
      asm_->cvtsi2sd(x86::xmm0, x86::rax);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_CONV_F64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->movsx(x86::rax, x86::eax);
      asm_->cvtsi2sd(x86::xmm0, x86::rax);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->cvtsi2sd(x86::xmm0, x86::rax);

      static_stack_alloc += 8;
      asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::BR: {
      asm_->jmp(basic_blocks[Type5InstructionReader(instr).Marg0()]);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));
      asm_->cmp(x86::rax, 1);

      asm_->je(basic_blocks[reader.Marg0()]);
      asm_->jmp(basic_blocks[reader.Marg1()]);
      return;
    }

    case Opcode::RETURN: {
      asm_->jmp(epilogue);
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type3InstructionReader reader(instr);

      if (type_manager.IsF64Type(static_cast<khir::Type>(reader.TypeID()))) {
        asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg()]));
      } else {
        asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));
      }

      asm_->jmp(epilogue);
      return;
    }

    case Opcode::FUNC_PTR: {
      Type3InstructionReader reader(instr);
      asm_->lea(x86::rax, x86::ptr(internal_func_labels_[reader.Arg()]));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);
      auto size =
          type_manager.GetTypeSize(static_cast<khir::Type>(reader.TypeID()));
      size += (16 - (size % 16)) % 16;
      assert(size % 16 == 0);

      asm_->sub(x86::rsp, size);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rsp);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I8_STORE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->mov(x86::ptr(x86::rax), x86::cl);
      return;
    }

    case Opcode::I16_STORE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->mov(x86::ptr(x86::rax), x86::cx);
      return;
    }

    case Opcode::I32_STORE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->mov(x86::ptr(x86::rax), x86::ecx);
      return;
    }

    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      asm_->mov(x86::ptr(x86::rax), x86::rcx);
      return;
    }

    case Opcode::I8_LOAD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->mov(x86::cl, x86::ptr(x86::rax));
      asm_->movzx(x86::rdx, x86::cl);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I16_LOAD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->mov(x86::cx, x86::ptr(x86::rax));
      asm_->movzx(x86::rdx, x86::cx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I32_LOAD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->mov(x86::ecx, x86::ptr(x86::rax));
      asm_->movzx(x86::rdx, x86::ecx);

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rdx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      asm_->mov(x86::rcx, x86::ptr(x86::rax));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rcx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));

      asm_->mov(x86::rcx, x86::ptr(x86::rax));

      static_stack_alloc += 8;
      asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rcx);
      offsets[instr_idx] = -static_stack_alloc;
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      const std::vector<x86::Gpq> regular_arg_regs = {
          x86::rdi, x86::rsi, x86::rdx, x86::rcx, x86::r8, x86::r9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      if (type_manager.IsF64Type(type)) {
        if (num_floating_point_args_ >= float_arg_regs.size()) {
          throw std::runtime_error(
              "Unsupported. Too many floating point args.");
        }

        static_stack_alloc += 8;
        asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc),
                    float_arg_regs[num_floating_point_args_]);
        offsets[instr_idx] = -static_stack_alloc;

        num_floating_point_args_++;
      } else {
        if (num_regular_args_ >= regular_arg_regs.size()) {
          throw std::runtime_error("Unsupported. Too many regular args.");
        }

        static_stack_alloc += 8;
        asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc),
                  regular_arg_regs[num_regular_args_]);
        offsets[instr_idx] = -static_stack_alloc;

        num_regular_args_++;
      }
      return;
    }

    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      if (type_manager.IsF64Type(type)) {
        floating_point_call_args_.emplace_back(offsets[reader.Arg()]);
      } else {
        regular_call_args_.emplace_back(offsets[reader.Arg()]);
      }

      return;
    }

    case Opcode::CALL:
    case Opcode::CALL_INDIRECT: {
      int64_t dynamic_stack_alloc = 0;
      const std::vector<x86::Gpq> regular_arg_regs = {
          x86::rdi, x86::rsi, x86::rdx, x86::rcx, x86::r8, x86::r9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      // pass f64 args
      int float_arg_idx = 0;
      while (float_arg_idx < floating_point_call_args_.size() &&
             float_arg_idx < float_arg_regs.size()) {
        asm_->movsd(
            float_arg_regs[float_arg_idx],
            x86::ptr(x86::rbp, floating_point_call_args_[float_arg_idx]));
        float_arg_idx++;
      }
      if (float_arg_idx < floating_point_call_args_.size()) {
        throw std::runtime_error("Too many floating point args for the call.");
      }

      // pass regular args
      int regular_arg_idx = 0;
      while (regular_arg_idx < regular_arg_regs.size() &&
             regular_arg_idx < regular_call_args_.size()) {
        asm_->mov(regular_arg_regs[regular_arg_idx],
                  x86::ptr(x86::rbp, regular_call_args_[regular_arg_idx]));
        regular_arg_idx++;
      }

      // if remaining args exist, pass them right to left
      int num_remaining_args = regular_call_args_.size() - regular_arg_idx;
      if (num_remaining_args > 0) {
        // add align space to remain 16 byte aligned
        if (num_remaining_args % 2 == 1) {
          asm_->push(x86::rax);
          dynamic_stack_alloc += 8;
        }

        for (int i = regular_call_args_.size() - 1; i >= regular_arg_idx; i--) {
          asm_->mov(x86::rax, x86::ptr(x86::rbp, regular_call_args_[i]));
          asm_->push(x86::rax);
          dynamic_stack_alloc += 8;
        }
      }
      regular_call_args_.clear();
      floating_point_call_args_.clear();
      assert(dynamic_stack_alloc % 16 == 0);

      khir::Type return_type;
      if (opcode == Opcode::CALL) {
        Type3InstructionReader reader(instr);
        const auto& func = functions[reader.Arg()];

        if (func.External()) {
          asm_->call(func.Addr());
        } else {
          asm_->call(internal_func_labels_[reader.Arg()]);
        }

        return_type = func.ReturnType();

      } else {
        Type3InstructionReader reader(instr);
        asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));
        asm_->call(x86::rax);

        return_type = type_manager.GetFunctionReturnType(
            static_cast<Type>(reader.TypeID()));
      }

      if (dynamic_stack_alloc != 0) {
        asm_->add(x86::rsp, dynamic_stack_alloc);
      }

      if (type_manager.IsVoid(return_type)) {
        return;
      } else if (type_manager.IsF64Type(return_type)) {
        static_stack_alloc += 8;
        asm_->movsd(x86::ptr(x86::rbp, -static_stack_alloc), x86::xmm0);
        offsets[instr_idx] = -static_stack_alloc;
        return;
      } else {
        static_stack_alloc += 8;
        asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
        offsets[instr_idx] = -static_stack_alloc;
        return;
      }
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      auto phi = reader.Arg0();
      auto phi_member = reader.Arg1();

      if (offsets[phi] == INT64_MAX) {
        asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[phi_member]));

        static_stack_alloc += 8;
        asm_->mov(x86::ptr(x86::rbp, -static_stack_alloc), x86::rax);
        offsets[phi] = -static_stack_alloc;
        return;
      } else {
        asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[phi_member]));
        asm_->mov(x86::ptr(x86::rbp, offsets[phi]), x86::rax);
        return;
      }
    }

    case Opcode::PHI: {
      // Noop since all handling is done in the phi member
      return;
    }
  }
}

void ASMBackend::Execute() {
  void* buffer_start;
  rt_.add(&buffer_start, &code_);

  auto offset = code_.labelOffsetFromBase(compute_label_);

  std::cerr << "Breakpoints:" << std::endl;
  for (const auto& [name, b_label] : breakpoints_) {
    auto b_offset = code_.labelOffsetFromBase(b_label);
    std::cerr << name << ": "
              << reinterpret_cast<void*>(
                     reinterpret_cast<uint64_t>(buffer_start) + b_offset)
              << std::endl;
  }

  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(
      reinterpret_cast<uint64_t>(buffer_start) + offset);
  compute();
}

}  // namespace kush::khir