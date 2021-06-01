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

    asm_->bind(internal_func_labels_[func_idx]);

    if (func.Name() == "compute") {
      compute_label_ = internal_func_labels_[func_idx];
    }

    // Prologue ================================================================
    // - Save all callee saved registers
    for (auto reg : callee_saved_registers) {
      asm_->push(reg);
    }
    // - Save RBP and Store RSP in RBP
    asm_->push(x86::rbp);
    asm_->mov(x86::rbp, x86::rsp);

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

    std::vector<int64_t> value_offsets(instructions.size(), 0);

    int64_t stack_size = 0;
    /*
    for (int i : basic_block_order) {
      asm_->bind(basic_blocks_impl[i]);
      const auto& [i_start, i_end] = basic_blocks[i];
      for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
        stack_size += TranslateInstr(type_manager, i64_constants, f64_constants,
                                     basic_blocks_impl, epilogue, value_offsets,
                                     instructions, instr_idx, stack_size);
      }
    }
    */

    // Update prologue to contain stack_size
    auto epilogue_offset = asm_->offset();
    asm_->setOffset(stack_alloc_offset);
    asm_->long_().sub(x86::rsp, stack_size);
    asm_->setOffset(epilogue_offset);

    // Epilogue ================================================================
    // - Restore RBP and dealloc stack space
    asm_->bind(epilogue);
    asm_->leave();

    // - Restore callee saved registers
    for (int i = callee_saved_registers.size() - 1; i >= 0; i--) {
      asm_->pop(callee_saved_registers[i]);
    }

    // - Return
    asm_->ret();
    // =========================================================================
  }
}

/*
x86::Gp GetWidthRegister(x86::Gpq full_size, int width) {
  int idx;
  if (width == 8) {
    idx = 0;
  } else if (width == 16) {
    idx = 1;
  } else if (width == 32) {
    idx = 2;
  } else if (width == 64) {
    idx = 3;
  } else {
    throw std::runtime_error("Invalid width.");
  }

  if (full_size == x86::rax) {
    return std::vector<x86::Gp>{x86::al, x86::ax, x86::eax, x86::rax}[idx];
  } else if (full_size == x86::rcx) {
    return std::vector<x86::Gp>{x86::cl, x86::cx, x86::ecx, x86::rcx}[idx];
  }

  throw std::runtime_error("Unimplemented.");
}

void ASMBackend::ComparisonInRax(Opcode op) {
  switch (op) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ:
    case Opcode::F64_CMP_EQ: {
      asm_->sete(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE:
    case Opcode::F64_CMP_NE: {
      asm_->setne(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT:
    case Opcode::F64_CMP_LT: {
      asm_->setl(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE:
    case Opcode::F64_CMP_LE: {
      asm_->setle(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT:
    case Opcode::F64_CMP_GT: {
      asm_->setg(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_CMP_GE: {
      asm_->setge(x86::al);
      asm_->movzx(x86::rax, x86::al);
      return;
    }
  }
}

int64_t ASMBackend::TranslateInstr(
    const TypeManager& type_manager, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<Label>& basic_blocks, const Label& epilogue,
    std::vector<int64_t>& offsets, const std::vector<uint64_t>& instructions,
    int instr_idx, int64_t current_stack_bottom) {
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
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), constant);
      offsets[instr_idx] = -current_stack_bottom;
      return 1;
    }

    case Opcode::I16_CONST: {
      auto constant = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), constant);
      offsets[instr_idx] = -current_stack_bottom;
      return 2;
    }

    case Opcode::I32_CONST: {
      auto constant = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), constant);
      offsets[instr_idx] = -current_stack_bottom;
      return 4;
    }

    case Opcode::I64_CONST: {
      auto i64_id = Type1InstructionReader(instr).Constant();
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom),
                i64_constants[i64_id]);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_CONST: {
      uint64_t f64_id = Type1InstructionReader(instr).Constant();
      double value = f64_constants[f64_id];

      uint64_t value_as_int;
      std::memcpy(&value_as_int, &value, sizeof(value_as_int));

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), value_as_int);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto char_id = Type1InstructionReader(instr).Constant();
      auto label = char_array_constants_[char_id];

      asm_->lea(x86::rax, x86::ptr(label));
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::NULLPTR: {
      uint64_t addr = 0;
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), addr);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::GLOBAL_REF: {
      auto glob_id = Type1InstructionReader(instr).Constant();
      auto label = globals_[glob_id];

      asm_->lea(x86::rax, x86::ptr(label));
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
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
      ComparisonInRax(opcode);

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
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
      ComparisonInRax(opcode);

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
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
      ComparisonInRax(opcode);

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
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
      ComparisonInRax(opcode);

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
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
      ComparisonInRax(opcode);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I8_ADD:
    case Opcode::I16_ADD:
    case Opcode::I32_ADD:
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      auto width = GetOperandWidth(opcode);
      asm_->add(GetWidthRegister(x86::rax, width),
                GetWidthRegister(x86::rcx, width));
      if (width != 64) {
        asm_->movzx(x86::rax, GetWidthRegister(x86::rax, width));
      }

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I8_MUL:
    case Opcode::I16_MUL:
    case Opcode::I32_MUL:
    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      auto width = GetOperandWidth(opcode);
      asm_->imul(GetWidthRegister(x86::rcx, width));
      if (width != 64) {
        asm_->movzx(x86::rax, GetWidthRegister(x86::rax, width));
      }

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I8_SUB:
    case Opcode::I16_SUB:
    case Opcode::I32_SUB:
    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      auto width = GetOperandWidth(opcode);
      asm_->sub(GetWidthRegister(x86::rax, width),
                GetWidthRegister(x86::rcx, width));
      if (width != 64) {
        asm_->movzx(x86::rax, GetWidthRegister(x86::rax, width));
      }

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I8_DIV:
    case Opcode::I16_DIV:
    case Opcode::I32_DIV:
    case Opcode::I64_DIV: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));

      auto width = GetOperandWidth(opcode);
      if (width == 16) {
        asm_->cwd();
      } else if (width == 32) {
        asm_->cdq();
      } else if (width == 64) {
        asm_->cqo();
      }
      asm_->idiv(GetWidthRegister(x86::rcx, width));
      if (width != 64) {
        asm_->movzx(x86::rax, GetWidthRegister(x86::rax, width));
      }

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));

      auto width = GetOperandWidth(opcode);
      asm_->movzx(x86::rax, GetWidthRegister(x86::rax, width));

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->xor_(x86::rax, 1);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->addsd(x86::xmm0, x86::xmm1);
      asm_->movsd(x86::ptr(x86::rbp, -current_stack_bottom), x86::xmm0);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->mulsd(x86::xmm0, x86::xmm1);
      asm_->movsd(x86::ptr(x86::rbp, -current_stack_bottom), x86::xmm0);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->subsd(x86::xmm0, x86::xmm1);
      asm_->movsd(x86::ptr(x86::rbp, -current_stack_bottom), x86::xmm0);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->movsd(x86::xmm1, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->divsd(x86::xmm0, x86::xmm1);
      asm_->movsd(x86::ptr(x86::rbp, -current_stack_bottom), x86::xmm0);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      asm_->movsd(x86::xmm0, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->cvttsd2si(x86::rax, x86::xmm0);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::I8_CONV_F64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      auto width = GetOperandWidth(opcode);

      if (width != 64) {
        asm_->movsx(x86::rax, GetWidthRegister(x86::rax, width));
      }
      asm_->cvtsi2sd(x86::xmm0, x86::rax);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::BR: {
      asm_->jmp(basic_blocks[Type5InstructionReader(instr).Marg0()]);
      return 0;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));
      asm_->cmp(x86::rax, 1);

      asm_->je(basic_blocks[reader.Marg0()]);
      asm_->jmp(basic_blocks[reader.Marg1()]);
      return 0;
    }

    case Opcode::RETURN: {
      asm_->jmp(epilogue);
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->jmp(epilogue);
      return;
    }

    case Opcode::STORE:
    case Opcode::LOAD: {
      // TODO
      return 0;
    }

    case Opcode::FUNC_PTR: {
      Type3InstructionReader reader(instr);
      asm_->movabs(x86::rax, internal_func_labels_[reader.Arg()]);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg()]));
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::PTR_ADD: {
      Type2InstructionReader reader(instr);
      asm_->mov(x86::rax, x86::ptr(x86::rbp, offsets[reader.Arg0()]));
      asm_->mov(x86::rcx, x86::ptr(x86::rbp, offsets[reader.Arg1()]));
      asm_->add(x86::rax, x86::rcx);
      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);
      auto size =
          type_manager.GetTypeSize(static_cast<khir::Type>(reader.TypeID()));
      asm_->mov(x86::rax, x86::rsp);
      size += (16 - (size % 16)) % 16;
      asm_->sub(x86::rsp, size);

      asm_->mov(x86::ptr(x86::rbp, -current_stack_bottom), x86::rax);
      offsets[instr_idx] = -current_stack_bottom;
      return 8;
    }

    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto v = values[reader.Arg()];
      call_args_.push_back(v);
      return;
    }

    case Opcode::CALL: {
      Type3InstructionReader reader(instr);
      auto func = functions_[reader.Arg()];
      values[instr_idx] = builder_->CreateCall(func, call_args_);
      call_args_.clear();
      return;
    }

    case Opcode::CALL_INDIRECT: {
      Type3InstructionReader reader(instr);
      auto func = values[reader.Arg()];
      auto func_type = types_[reader.TypeID()];
      values[instr_idx] = llvm::CallInst::Create(
          llvm::dyn_cast<llvm::FunctionType>(func_type), func, call_args_, "",
          builder_->GetInsertBlock());
      call_args_.clear();
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto arg_idx = reader.Sarg();
      values[instr_idx] = func_args[arg_idx];
      return;
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      auto phi = values[reader.Arg0()];
      auto phi_member = values[reader.Arg1()];

      if (phi == nullptr) {
        phi_member_list[reader.Arg0()].emplace_back(phi_member,
                                                    builder_->GetInsertBlock());
      } else {
        llvm::dyn_cast<llvm::PHINode>(phi)->addIncoming(
            phi_member, builder_->GetInsertBlock());
      }
      return;
    }

    case Opcode::PHI: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];

      auto phi = builder_->CreatePHI(t, 2);
      values[instr_idx] = phi;

      if (phi_member_list.contains(instr_idx)) {
        for (const auto& [phi_member, basic_block] :
             phi_member_list[instr_idx]) {
          phi->addIncoming(phi_member, basic_block);
        }

        phi_member_list.erase(instr_idx);
      }

      return;
    }
  }
}
*/

void ASMBackend::Execute() {
  void* buffer_start;
  rt_.add(&buffer_start, &code_);

  auto offset = code_.labelOffsetFromBase(compute_label_);

  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(
      reinterpret_cast<uint64_t>(buffer_start) + offset);
  compute();
}

}  // namespace kush::khir