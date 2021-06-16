#include "khir/asm/asm_backend.h"

#include <iostream>
#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "khir/asm/linear_scan_register_alloc.h"
#include "khir/asm/live_intervals.h"
#include "khir/instruction.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

StackSlotAllocator::StackSlotAllocator(int32_t initial_size)
    : size_(initial_size) {}

int32_t StackSlotAllocator::AllocateSlot() {
  size_ += 8;
  return -size_;
}

int32_t StackSlotAllocator::GetSize() { return size_; }

using namespace asmjit;

void ExceptionErrorHandler::handleError(Error err, const char* message,
                                        BaseEmitter* origin) {
  throw std::runtime_error(message);
}

uint64_t ASMBackend::OutputConstant(
    uint64_t instr, const TypeManager& type_manager,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals) {
  auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case ConstantOpcode::I1_CONST:
    case ConstantOpcode::I8_CONST: {
      // treat I1 as I8
      uint8_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt8(constant);
      return 1;
    }

    case ConstantOpcode::I16_CONST: {
      uint16_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt16(constant);
      return 2;
    }

    case ConstantOpcode::I32_CONST: {
      uint32_t constant = Type1InstructionReader(instr).Constant();
      asm_->embedUInt32(constant);
      return 4;
    }

    case ConstantOpcode::I64_CONST: {
      auto i64_id = Type1InstructionReader(instr).Constant();
      asm_->embedUInt64(i64_constants[i64_id]);
      return 8;
    }

    case ConstantOpcode::F64_CONST: {
      auto f64_id = Type1InstructionReader(instr).Constant();
      asm_->embedDouble(f64_constants[f64_id]);
      return 8;
    }

    case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto str_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(char_array_constants_[str_id]);
      return 8;
    }

    case ConstantOpcode::NULLPTR: {
      asm_->embedUInt64(0);
      return 8;
    }

    case ConstantOpcode::GLOBAL_REF: {
      auto global_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(globals_[global_id]);
      return 8;
    }

    case ConstantOpcode::FUNC_PTR: {
      asm_->embedLabel(
          internal_func_labels_[Type3InstructionReader(instr).Arg()]);
      return 8;
    }

    case ConstantOpcode::STRUCT_CONST: {
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

        auto field_const_instr = constant_instrs[field_const.GetIdx()];

        bytes_written +=
            OutputConstant(field_const_instr, type_manager, constant_instrs,
                           i64_constants, f64_constants, char_array_constants,
                           struct_constants, array_constants, globals);
      }

      while (bytes_written < type_size) {
        asm_->embedUInt8(0);
        bytes_written++;
      }

      return bytes_written;
    }

    case ConstantOpcode::ARRAY_CONST: {
      auto arr_id = Type1InstructionReader(instr).Constant();
      const auto& arr_const = array_constants[arr_id];

      uint64_t bytes_written = 0;
      for (auto elem_const : arr_const.Elements()) {
        auto elem_const_instr = constant_instrs[elem_const.GetIdx()];
        // arrays have no padding
        bytes_written +=
            OutputConstant(elem_const_instr, type_manager, constant_instrs,
                           i64_constants, f64_constants, char_array_constants,
                           struct_constants, array_constants, globals);
      }
      return bytes_written;
    }
  }
}

void ASMBackend::Translate(const TypeManager& type_manager,
                           const std::vector<uint64_t>& i64_constants,
                           const std::vector<double>& f64_constants,
                           const std::vector<std::string>& char_array_constants,
                           const std::vector<StructConstant>& struct_constants,
                           const std::vector<ArrayConstant>& array_constants,
                           const std::vector<Global>& globals,
                           const std::vector<uint64_t>& constant_instrs,
                           const std::vector<Function>& functions) {
  start = std::chrono::system_clock::now();
  code_.init(rt_.environment());
  asm_ = std::make_unique<x86::Assembler>(&code_);

  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);

  asm_->section(data_section_);

  // Declare all functions
  for (const auto& function : functions) {
    external_func_addr_.push_back(function.Addr());
    internal_func_labels_.push_back(asm_->newLabel());
  }

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
    auto v = global.InitialValue();
    OutputConstant(constant_instrs[v.GetIdx()], type_manager, constant_instrs,
                   i64_constants, f64_constants, char_array_constants,
                   struct_constants, array_constants, globals);
  }

  asm_->section(text_section_);

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

    auto live_intervals = ComputeLiveIntervals(func, type_manager);
    auto register_assign = AssignRegisters(live_intervals);

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
    StackSlotAllocator static_stack_allocator(8 *
                                              callee_saved_registers.size());
    for (int i : basic_block_order) {
      asm_->bind(basic_blocks_impl[i]);
      const auto& [i_start, i_end] = basic_blocks[i];
      for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
        TranslateInstr(type_manager, i64_constants, f64_constants,
                       basic_blocks_impl, functions, epilogue, value_offsets,
                       instructions, constant_instrs, instr_idx,
                       static_stack_allocator, register_assign);
      }
    }

    // we need to ensure that it is aligned to 16 bytes so if we call any
    // function, stack is 16 byte aligned
    auto static_stack_frame_size = static_stack_allocator.GetSize();
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
    asm_->bind(epilogue);

    // - Restore callee saved registers
    for (int i = callee_saved_registers.size() - 1; i >= 0; i--) {
      asm_->mov(callee_saved_registers[i],
                x86::qword_ptr(x86::rbp, -8 * (i + 1)));
    }

    // - Restore RSP into RBP and Restore RBP and Store RSP in RBP
    asm_->mov(x86::rsp, x86::rbp);
    asm_->pop(x86::rbp);

    // - Return
    asm_->ret();
    // =========================================================================
  }
}

asmjit::Label ASMBackend::GetConstantGlobal(uint64_t instr) {
  auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());
  switch (opcode) {
    case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto id = Type1InstructionReader(instr).Constant();
      return char_array_constants_[id];
    }

    case ConstantOpcode::GLOBAL_REF: {
      auto id = Type1InstructionReader(instr).Constant();
      return globals_[id];
    }

    case ConstantOpcode::FUNC_PTR: {
      auto id = Type3InstructionReader(instr).Arg();
      return internal_func_labels_[id];
    }

    default:
      throw std::runtime_error("Invalid constant global.");
  }
}

bool ASMBackend::IsGep(khir::Value v,
                       const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }
  return OpcodeFrom(
             GenericInstructionReader(instructions[v.GetIdx()]).Opcode()) ==
         Opcode::GEP;
}

std::pair<khir::Value, int32_t> ASMBackend::Gep(
    khir::Value v, const std::vector<uint64_t>& instructions,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants) {
  Type3InstructionReader gep_reader(instructions[v.GetIdx()]);
  if (OpcodeFrom(gep_reader.Opcode()) != Opcode::GEP) {
    throw std::runtime_error("Invalid GEP");
  }

  auto gep_offset_instr = instructions[khir::Value(gep_reader.Arg()).GetIdx()];
  Type2InstructionReader gep_offset_reader(gep_offset_instr);
  if (OpcodeFrom(gep_offset_reader.Opcode()) != Opcode::GEP_OFFSET) {
    throw std::runtime_error("Invalid GEP Offset");
  }

  auto ptr = khir::Value(gep_offset_reader.Arg0());

  auto constant_value = khir::Value(gep_offset_reader.Arg1());
  if (!constant_value.IsConstantGlobal()) {
    throw std::runtime_error("Invalid GEP offset");
  }
  int32_t offset =
      Type1InstructionReader(constant_instrs[constant_value.GetIdx()])
          .Constant();
  return {ptr, offset};
}

void ASMBackend::TranslateInstr(
    const TypeManager& type_manager, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<Label>& basic_blocks,
    const std::vector<Function>& functions, const Label& epilogue,
    std::vector<int64_t>& offsets, const std::vector<uint64_t>& instructions,
    const std::vector<uint64_t>& constant_instrs, int instr_idx,
    StackSlotAllocator& stack_allocator,
    const std::vector<int>& register_assign) {
  auto instr = instructions[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  /*
Available for allocation:
 RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
 XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7

// Reserved/Scratch
 RSP, RBP, RAX, XMM0
*/
  static std::vector<Register> normal_registers{
      Register::RBX, Register::RCX, Register::RDX, Register::RSI, Register::RDI,
      Register::R8,  Register::R9,  Register::R10, Register::R11, Register::R12,
      Register::R13, Register::R14, Register::R15};
  static std::vector<x86::Xmm> fp_registers{x86::xmm1, x86::xmm2, x86::xmm3,
                                            x86::xmm4, x86::xmm5, x86::xmm6,
                                            x86::xmm7};

  bool dest_is_reg = register_assign[instr_idx] >= 0;
  int dest_reg = register_assign[instr_idx];

  switch (opcode) {
    // I1
    // ======================================================================
    case Opcode::I1_ZEXT_I8: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int8_t constant =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      int32_t offset;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v.IsConstantGlobal() && dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(), constant);
      } else if (v.IsConstantGlobal() && !dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), constant);
      } else if (v_is_reg && dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(),
                  normal_registers[v_reg].GetB());
      } else if (v_is_reg && !dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset),
                  normal_registers[v_reg].GetB());
      } else if (dest_is_reg) {  // v on stack
        asm_->mov(normal_registers[dest_reg].GetB(),
                  (x86::byte_ptr(x86::rbp, offsets[v.GetIdx()])));
      } else {  // v on stack, dest on stack
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int8_t constant =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      int32_t offset;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v.IsConstantGlobal() && dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(), constant ^ 1);
      } else if (v.IsConstantGlobal() && !dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), constant ^ 1);
      } else if (v_is_reg && dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(),
                  normal_registers[v_reg].GetB());
        asm_->xor_(normal_registers[dest_reg].GetB(), 1);
      } else if (v_is_reg && !dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset),
                  normal_registers[v_reg].GetB());
        asm_->xor_(x86::byte_ptr(x86::rbp, offset), 1);
      } else if (dest_is_reg) {  // v on stack
        asm_->mov(normal_registers[dest_reg].GetB(),
                  (x86::byte_ptr(x86::rbp, offsets[v.GetIdx()])));
        asm_->xor_(normal_registers[dest_reg].GetB(), 1);
      } else {  // v on stack, dest on stack
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
        asm_->xor_(x86::al, 1);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    // I8 ======================================================================
    case Opcode::I8_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int8_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int8_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetB() : x86::al;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetB());
      } else {
        asm_->mov(dest, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (v1.IsConstantGlobal()) {
        asm_->add(dest, c1);
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetB());
      } else {
        asm_->add(dest, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    case Opcode::I8_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int8_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int8_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetB() : x86::al;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetB());
      } else {
        asm_->mov(dest, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (v1.IsConstantGlobal()) {
        asm_->sub(dest, c1);
      } else if (v1_is_reg) {
        asm_->sub(dest, normal_registers[v1_reg].GetB());
      } else {
        asm_->sub(dest, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    case Opcode::I8_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int8_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int8_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        asm_->mov(x86::ax, c0);
      } else if (v0_is_reg) {
        asm_->movzx(x86::ax, normal_registers[v0_reg].GetB());
      } else {
        asm_->movzx(x86::ax, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (v1.IsConstantGlobal()) {
        asm_->imul(x86::ax, x86::ax, c1);
      } else if (v1_is_reg) {
        asm_->imul(normal_registers[v1_reg].GetB());
      } else {
        asm_->imul(x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(), x86::al);
      } else {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    case Opcode::I8_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 / c1;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        auto v_offset = offsets[v1.GetIdx()];

        int32_t sexted = c;
        asm_->mov(x86::eax, sexted);
        asm_->cwd();
        asm_->movsx(x86::ecx, x86::byte_ptr(x86::rbp, v_offset));
        asm_->idiv(x86::cx);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        int8_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();

        int32_t sexted = c;
        asm_->movsx(x86::eax, x86::byte_ptr(x86::rbp, v_offset));
        asm_->cwd();
        asm_->mov(x86::ecx, sexted);
        asm_->idiv(x86::cx);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      } else {
        asm_->movsx(x86::eax, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cwd();
        asm_->movsx(x86::ecx, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->idiv(x86::cx);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 == c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int8_t c = v0.IsConstantGlobal()
                       ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                             .Constant()
                       : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                             .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::byte_ptr(x86::rbp, v_offset), c);
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 != c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int8_t c = v0.IsConstantGlobal()
                       ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                             .Constant()
                       : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                             .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::byte_ptr(x86::rbp, v_offset), c);
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_CMP_LT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 < c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();

        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_CMP_LE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 <= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();

        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_CMP_GT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();

        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int8_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int8_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 >= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();

        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::al, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        uint8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        uint32_t res = c;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), res);
      } else {
        asm_->movsx(x86::rax, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        double res = c;
        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));

        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->movsx(x86::eax, x86::byte_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cvtsi2sd(x86::xmm0, x86::eax);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I8_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::byte_ptr(label, ptr_offset), c);
        } else {
          asm_->mov(x86::cl, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::byte_ptr(label, ptr_offset), x86::cl);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::byte_ptr(x86::rax, ptr_offset), c);
        } else {
          asm_->mov(x86::cl, x86::byte_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::byte_ptr(x86::rax, ptr_offset), x86::cl);
        }
      }
      return;
    }

    case Opcode::I8_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::al, x86::byte_ptr(label, ptr_offset));
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::al, x86::byte_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      offsets[instr_idx] = offset;
      return;
    }

    // I16 =====================================================================
    case Opcode::I16_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int16_t res = c0 + c1;
        asm_->mov(x86::word_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int16_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->mov(x86::eax, uint32_t(c));
        asm_->add(x86::ax, x86::word_ptr(x86::rbp, v_offset));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->add(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int16_t res = c0 - c1;
        asm_->mov(x86::word_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->mov(x86::eax, c);
        asm_->sub(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else if (v1.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->sub(x86::ax, c);
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->sub(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int16_t res = c0 * c1;
        asm_->mov(x86::word_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int16_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->imul(x86::ax, x86::word_ptr(x86::rbp, v_offset), c);
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->imul(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int16_t res = c0 / c1;
        asm_->mov(x86::word_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        auto v_offset = offsets[v1.GetIdx()];
        int32_t sexted = c;
        asm_->mov(x86::eax, sexted);
        asm_->movsx(x86::ecx, x86::word_ptr(x86::rbp, v_offset));
        asm_->cdq();
        asm_->idiv(x86::ecx);
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int32_t sexted = c;
        asm_->movsx(x86::eax, x86::word_ptr(x86::rbp, v_offset));
        asm_->mov(x86::ecx, sexted);
        asm_->cdq();
        asm_->idiv(x86::ecx);
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else {
        asm_->movsx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->movsx(x86::ecx, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->cdq();
        asm_->idiv(x86::ecx);
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_EQ: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 == c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int16_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::word_ptr(x86::rbp, v_offset), c);
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_NE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 != c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int16_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::word_ptr(x86::rbp, v_offset), c);
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_LT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 < c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_LE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 <= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_GT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int16_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int16_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 >= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::ax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        uint16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        uint32_t res = c;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), res);
      } else {
        asm_->movzx(x86::rax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        int16_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        double res = c;
        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));

        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->movsx(x86::eax, x86::word_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cvtsi2sd(x86::xmm0, x86::eax);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I16_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::word_ptr(label, ptr_offset), c);
        } else {
          asm_->movzx(x86::eax, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::word_ptr(label, ptr_offset), x86::ax);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::word_ptr(x86::rax, ptr_offset), c);
        } else {
          asm_->movzx(x86::ecx, x86::word_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::word_ptr(x86::rax, ptr_offset), x86::cx);
        }
      }
      return;
    }

    case Opcode::I16_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::ax, x86::word_ptr(label, ptr_offset));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::ax, x86::word_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    // I32 =====================================================================
    case Opcode::I32_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int32_t res = c0 + c1;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int32_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->mov(x86::eax, c);
        asm_->add(x86::eax, x86::dword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->add(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int32_t res = c0 - c1;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->mov(x86::eax, c);
        asm_->sub(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else if (v1.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->sub(x86::eax, c);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->sub(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int32_t res = c0 * c1;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int32_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->imul(x86::eax, x86::dword_ptr(x86::rbp, v_offset), c);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->imul(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int32_t res = c0 / c1;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        auto v_offset = offsets[v1.GetIdx()];
        asm_->mov(x86::eax, c);
        asm_->cdq();
        asm_->idiv(x86::dword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::ecx, c);
        asm_->cdq();
        asm_->idiv(x86::ecx);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cdq();
        asm_->idiv(x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_EQ: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 == c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int32_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::dword_ptr(x86::rbp, v_offset), c);
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_NE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 != c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int32_t c = v0.IsConstantGlobal()
                        ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()
                        : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant();
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->cmp(x86::dword_ptr(x86::rbp, v_offset), c);
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_LT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 < c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_LE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 <= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_GT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int32_t c0 =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        int32_t c1 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        int8_t res = c0 >= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]), c);
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->cmp(x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]), c);
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        uint32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        asm_->mov(x86::rax, c);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->movzx(x86::rax, x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        int32_t c =
            Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant();
        double res = c;
        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));

        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->cvtsi2sd(x86::xmm0,
                       x86::dword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I32_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::dword_ptr(label, ptr_offset), c);
        } else {
          asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::dword_ptr(label, ptr_offset), x86::eax);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
          asm_->mov(x86::dword_ptr(x86::rax, ptr_offset), c);
        } else {
          asm_->mov(x86::ecx, x86::dword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::dword_ptr(x86::rax, ptr_offset), x86::ecx);
        }
      }
      return;
    }

    case Opcode::I32_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::eax, x86::dword_ptr(label, ptr_offset));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::ecx, x86::dword_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::ecx);
      }
      offsets[instr_idx] = offset;
      return;
    }

    // I64 =====================================================================
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int64_t res = c0 + c1;
        asm_->mov(x86::rax, res);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int64_t c = i64_constants
            [v0.IsConstantGlobal()
                 ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                       .Constant()
                 : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                       .Constant()];
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->mov(x86::rax, c);
        asm_->add(x86::rax, x86::qword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->add(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int64_t res = c0 - c1;
        asm_->mov(x86::rax, res);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->sub(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v1.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::rcx, c);
        asm_->sub(x86::rax, x86::rcx);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->sub(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int64_t res = c0 * c1;
        asm_->mov(x86::rax, res);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int64_t c = i64_constants
            [v0.IsConstantGlobal()
                 ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                       .Constant()
                 : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                       .Constant()];
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];
        asm_->mov(x86::rax, c);
        asm_->imul(x86::rax, x86::qword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->imul(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int64_t res = c0 / c1;
        asm_->mov(x86::rax, res);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];
        asm_->mov(x86::rax, c);
        asm_->cqo();
        asm_->idiv(x86::qword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, v_offset));
        asm_->mov(x86::rcx, c);
        asm_->cqo();
        asm_->idiv(x86::rcx);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cqo();
        asm_->idiv(x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_EQ: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 == c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int64_t c = i64_constants
            [v0.IsConstantGlobal()
                 ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                       .Constant()
                 : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                       .Constant()];
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, v_offset));
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->sete(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_NE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 != c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal() || v1.IsConstantGlobal()) {
        int64_t c = i64_constants
            [v0.IsConstantGlobal()
                 ? Type1InstructionReader(constant_instrs[v0.GetIdx()])
                       .Constant()
                 : Type1InstructionReader(constant_instrs[v1.GetIdx()])
                       .Constant()];
        auto v_offset =
            v0.IsConstantGlobal() ? offsets[v1.GetIdx()] : offsets[v0.GetIdx()];

        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, v_offset));
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setne(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_LT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 < c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_LE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 <= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_GT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->setl(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setg(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        int64_t c0 =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        int64_t c1 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 >= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
      } else if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      } else if (v1.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->mov(x86::rax, c);
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->setle(x86::byte_ptr(x86::rbp, offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->cmp(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->setge(x86::byte_ptr(x86::rbp, offset));
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        int64_t c =
            i64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double res = c;
        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));
        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->cvtsi2sd(x86::xmm0,
                       x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::I64_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                                .Constant()];
          asm_->mov(x86::rax, c);
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                                .Constant()];
          asm_->mov(x86::rcx, c);
          asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
        } else {
          asm_->mov(x86::rcx, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
        }
      }
      return;
    }

    case Opcode::I64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::rcx, x86::qword_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rcx);
      }
      offsets[instr_idx] = offset;
      return;
    }

    // PTR =====================================================================
    case Opcode::GEP:
    case Opcode::GEP_OFFSET: {
      // do nothing since we lazily compute GEPs
      return;
    }

    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);
      auto size =
          type_manager.GetTypeSize(static_cast<khir::Type>(reader.TypeID()));
      size += (16 - (size % 16)) % 16;
      assert(size % 16 == 0);

      auto offset = stack_allocator.AllocateSlot();
      asm_->sub(x86::rsp, size);
      asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rsp);
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());
      int32_t ptr_offset = 0;

      if (IsGep(v, instructions)) {
        auto [ptr, o] = Gep(v, instructions, constant_instrs, i64_constants);
        v = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v.IsConstantGlobal()) {
        if (ConstantOpcodeFrom(
                GenericInstructionReader(constant_instrs[v.GetIdx()])
                    .Opcode()) == ConstantOpcode::NULLPTR) {
          asm_->mov(x86::rax, 0);
        } else {
          auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
          asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
        }
      } else {
        if (ptr_offset != 0) {
          asm_->mov(x86::rdx, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->lea(x86::rax, x86::ptr(x86::rdx, ptr_offset));
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      }
      asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          if (ConstantOpcodeFrom(
                  GenericInstructionReader(constant_instrs[v1.GetIdx()])
                      .Opcode()) == ConstantOpcode::NULLPTR) {
            asm_->mov(x86::qword_ptr(label, ptr_offset), 0);
          } else {
            auto v_label = GetConstantGlobal(constant_instrs[v1.GetIdx()]);
            asm_->lea(x86::rax, x86::ptr(v_label));
            asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
          }
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          if (ConstantOpcodeFrom(
                  GenericInstructionReader(constant_instrs[v1.GetIdx()])
                      .Opcode()) == ConstantOpcode::NULLPTR) {
            asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), 0);
          } else {
            auto label = GetConstantGlobal(constant_instrs[v1.GetIdx()]);
            asm_->lea(x86::rcx, x86::ptr(label));
            asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
          }
        } else {
          asm_->mov(x86::rcx, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
        }
      }
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      Value v0(reader.Arg());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::rcx, x86::qword_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rcx);
      }
      offsets[instr_idx] = offset;
      return;
    }

    // F64 =====================================================================
    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        double res = c0 + c1;

        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));
        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->addsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->addsd(x86::xmm0, x86::qword_ptr(label));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->addsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        double res = c0 - c1;

        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));
        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->subsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->subsd(x86::xmm0, x86::qword_ptr(label));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->subsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        double res = c0 * c1;

        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));
        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->mulsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->mulsd(x86::xmm0, x86::qword_ptr(label));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mulsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        double res = c0 / c1;

        uint64_t res_as_int;
        std::memcpy(&res_as_int, &res, sizeof(res_as_int));
        asm_->mov(x86::rax, res_as_int);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->divsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->divsd(x86::xmm0, x86::qword_ptr(label));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->divsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::F64_CMP_EQ: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 == c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
      }
      asm_->setnp(x86::al);
      asm_->sete(x86::cl);
      asm_->and_(x86::cl, x86::al);
      asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::cl);
      return;
    }

    case Opcode::F64_CMP_NE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 != c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
      }
      asm_->setnp(x86::al);
      asm_->setne(x86::cl);
      asm_->or_(x86::cl, x86::al);
      asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::cl);
      return;
    }

    case Opcode::F64_CMP_LT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 < c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }
      asm_->seta(x86::byte_ptr(x86::rbp, offset));
      return;
    }

    case Opcode::F64_CMP_LE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 <= c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
      }
      asm_->setae(x86::byte_ptr(x86::rbp, offset));
      return;
    }

    case Opcode::F64_CMP_GT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
      }
      asm_->seta(x86::byte_ptr(x86::rbp, offset));
      return;
    }

    case Opcode::F64_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v0.IsConstantGlobal() && v1.IsConstantGlobal()) {
        double c0 =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        double c1 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        int8_t res = c0 > c1 ? 1 : 0;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), res);
        return;
      }

      if (v0.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v0.GetIdx()])
                              .Constant()];
        auto v_offset = offsets[v1.GetIdx()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
      } else if (v1.IsConstantGlobal()) {
        auto v_offset = offsets[v0.GetIdx()];
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];

        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c);
        asm_->section(text_section_);

        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, v_offset));
        asm_->ucomisd(x86::xmm0, x86::qword_ptr(label));
      } else {
        asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->ucomisd(x86::xmm0,
                      x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
      }
      asm_->setae(x86::byte_ptr(x86::rbp, offset));
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto offset = stack_allocator.AllocateSlot();
      if (v.IsConstantGlobal()) {
        double c =
            f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant()];
        int64_t conv = c;
        asm_->mov(x86::rax, conv);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->cvttsd2si(x86::rax,
                        x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::F64_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);

        if (v1.IsConstantGlobal()) {
          double c =
              f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                                .Constant()];
          uint64_t res_as_int;
          std::memcpy(&res_as_int, &c, sizeof(res_as_int));
          asm_->mov(x86::rax, res_as_int);
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));

        if (v1.IsConstantGlobal()) {
          double c =
              f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                                .Constant()];
          uint64_t res_as_int;
          std::memcpy(&res_as_int, &c, sizeof(res_as_int));
          asm_->mov(x86::rcx, res_as_int);
          asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
        } else {
          asm_->mov(x86::rcx, x86::qword_ptr(x86::rbp, offsets[v1.GetIdx()]));
          asm_->mov(x86::qword_ptr(x86::rax, ptr_offset), x86::rcx);
        }
      }
      return;
    }

    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      int32_t ptr_offset = 0;

      if (IsGep(v0, instructions)) {
        auto [ptr, o] = Gep(v0, instructions, constant_instrs, i64_constants);
        v0 = ptr;
        ptr_offset = o;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v0.GetIdx()]));
        asm_->mov(x86::rcx, x86::qword_ptr(x86::rax, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rcx);
      }

      offsets[instr_idx] = offset;
      return;
    }

    // Control Flow ============================================================
    case Opcode::BR: {
      asm_->jmp(basic_blocks[Type5InstructionReader(instr).Marg0()]);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      khir::Value v(reader.Arg());

      if (v.IsConstantGlobal()) {
        auto c =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant() != 0;
        if (c) {
          asm_->jmp(basic_blocks[reader.Marg0()]);
        } else {
          asm_->jmp(basic_blocks[reader.Marg1()]);
        }
      } else {
        asm_->cmp(x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]), 1);
        asm_->je(basic_blocks[reader.Marg0()]);
        asm_->jmp(basic_blocks[reader.Marg1()]);
      }

      return;
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      Value phi(reader.Arg0());
      Value v(reader.Arg1());
      auto type = static_cast<khir::Type>(
          Type3InstructionReader(instructions[phi.GetIdx()]).TypeID());

      auto offset = offsets[phi.GetIdx()];
      if (offsets[phi.GetIdx()] == INT64_MAX) {
        offset = stack_allocator.AllocateSlot();
        offsets[phi.GetIdx()] = offset;
      }

      if (type_manager.IsF64Type(type)) {
        if (v.IsConstantGlobal()) {
          double c =
              f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          uint64_t res_as_int;
          std::memcpy(&res_as_int, &c, sizeof(res_as_int));
          asm_->mov(x86::rax, res_as_int);
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        }
      } else if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (v.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::byte_ptr(x86::rbp, offset), c);
        } else {
          asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
        }
      } else if (type_manager.IsI16Type(type)) {
        if (v.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::word_ptr(x86::rbp, offset), c);
        } else {
          asm_->mov(x86::ax, x86::word_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
        }
      } else if (type_manager.IsI32Type(type)) {
        if (v.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::dword_ptr(x86::rbp, offset), c);
        } else {
          asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
        }
      } else if (type_manager.IsI64Type(type)) {
        if (v.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          asm_->mov(x86::rax, c);
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        }
      } else if (type_manager.IsPtrType(type)) {
        int32_t ptr_offset = 0;

        if (IsGep(v, instructions)) {
          auto [ptr, o] = Gep(v, instructions, constant_instrs, i64_constants);
          v = ptr;
          ptr_offset = o;
        }

        if (v.IsConstantGlobal()) {
          if (ConstantOpcodeFrom(
                  GenericInstructionReader(constant_instrs[v.GetIdx()])
                      .Opcode()) == ConstantOpcode::NULLPTR) {
            asm_->mov(x86::qword_ptr(x86::rbp, offset), 0);
          } else {
            auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
            asm_->lea(x86::rcx, x86::ptr(label, ptr_offset));
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rcx);
          }
        } else {
          asm_->mov(x86::rcx, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          if (ptr_offset != 0) {
            asm_->mov(x86::rdx, ptr_offset);
            asm_->add(x86::rcx, x86::rdx);
          }
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rcx);
        }
      } else {
        throw std::runtime_error("Invalid argument type.");
      }
    }

    case Opcode::PHI: {
      // Noop since all handling is done in the phi member
      return;
    }

    // Calls ===================================================================
    case Opcode::RETURN: {
      asm_->jmp(epilogue);
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type3InstructionReader reader(instr);
      khir::Value v(reader.Arg());
      auto type = static_cast<khir::Type>(reader.TypeID());

      if (type_manager.IsF64Type(type)) {
        if (v.IsConstantGlobal()) {
          double c =
              f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          auto label = asm_->newLabel();
          asm_->section(data_section_);
          asm_->bind(label);
          asm_->embedDouble(c);
          asm_->section(text_section_);
          asm_->movsd(x86::xmm0, x86::qword_ptr(label));
        } else {
          asm_->movsd(x86::xmm0, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      } else if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (v.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::al, c);
        } else {
          asm_->mov(x86::al, x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      } else if (type_manager.IsI16Type(type)) {
        if (v.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::ax, c);
        } else {
          asm_->mov(x86::ax, x86::word_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      } else if (type_manager.IsI32Type(type)) {
        if (v.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::eax, c);
        } else {
          asm_->mov(x86::eax, x86::dword_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      } else if (type_manager.IsI64Type(type)) {
        if (v.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          asm_->mov(x86::rax, c);
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
      } else if (type_manager.IsPtrType(type)) {
        int32_t ptr_offset = 0;

        if (IsGep(v, instructions)) {
          auto [ptr, o] = Gep(v, instructions, constant_instrs, i64_constants);
          v = ptr;
          ptr_offset = o;
        }

        if (v.IsConstantGlobal()) {
          if (ConstantOpcodeFrom(
                  GenericInstructionReader(constant_instrs[v.GetIdx()])
                      .Opcode()) == ConstantOpcode::NULLPTR) {
            asm_->mov(x86::rax, 0);
          } else {
            auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
            asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
          }
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          if (ptr_offset != 0) {
            asm_->mov(x86::rdx, ptr_offset);
            asm_->add(x86::rax, x86::rdx);
          }
        }
      } else {
        throw std::runtime_error("Invalid return value type.");
      }
      asm_->jmp(epilogue);
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      const std::vector<x86::Gpb> byte_arg_regs = {
          x86::dil, x86::sil, x86::dl, x86::cl, x86::r8b, x86::r9b};
      const std::vector<x86::Gpw> word_arg_regs = {x86::di, x86::si,  x86::dx,
                                                   x86::cx, x86::r8w, x86::r9w};
      const std::vector<x86::Gpd> dword_arg_regs = {
          x86::edi, x86::esi, x86::edx, x86::ecx, x86::r8d, x86::r9d};
      const std::vector<x86::Gpq> qword_arg_regs = {
          x86::rdi, x86::rsi, x86::rdx, x86::rcx, x86::r8, x86::r9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      auto offset = stack_allocator.AllocateSlot();
      if (type_manager.IsF64Type(type)) {
        if (num_floating_point_args_ >= float_arg_regs.size()) {
          throw std::runtime_error(
              "Unsupported. Too many floating point args.");
        }

        asm_->movsd(x86::qword_ptr(x86::rbp, offset),
                    float_arg_regs[num_floating_point_args_]);
        num_floating_point_args_++;
      } else {
        if (num_regular_args_ >= qword_arg_regs.size()) {
          throw std::runtime_error("Unsupported. Too many regular args.");
        }

        if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
          asm_->mov(x86::byte_ptr(x86::rbp, offset),
                    byte_arg_regs[num_regular_args_]);
        } else if (type_manager.IsI16Type(type)) {
          asm_->mov(x86::word_ptr(x86::rbp, offset),
                    word_arg_regs[num_regular_args_]);
        } else if (type_manager.IsI32Type(type)) {
          asm_->mov(x86::dword_ptr(x86::rbp, offset),
                    dword_arg_regs[num_regular_args_]);
        } else if (type_manager.IsI64Type(type) ||
                   type_manager.IsPtrType(type)) {
          asm_->mov(x86::qword_ptr(x86::rbp, offset),
                    qword_arg_regs[num_regular_args_]);
        } else {
          throw std::runtime_error("Invalid argument type.");
        }

        num_regular_args_++;
      }
      offsets[instr_idx] = offset;
      return;
    }

    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      if (type_manager.IsF64Type(type)) {
        floating_point_call_args_.emplace_back(reader.Arg());
      } else {
        regular_call_args_.emplace_back(khir::Value(reader.Arg()), type);
      }
      return;
    }

    case Opcode::CALL:
    case Opcode::CALL_INDIRECT: {
      int64_t dynamic_stack_alloc = 0;
      const std::vector<x86::Gpb> byte_arg_regs = {
          x86::dil, x86::sil, x86::dl, x86::cl, x86::r8b, x86::r9b};
      const std::vector<x86::Gpw> word_arg_regs = {x86::di, x86::si,  x86::dx,
                                                   x86::cx, x86::r8w, x86::r9w};
      const std::vector<x86::Gpd> dword_arg_regs = {
          x86::edi, x86::esi, x86::edx, x86::ecx, x86::r8d, x86::r9d};
      const std::vector<x86::Gpq> qword_arg_regs = {
          x86::rdi, x86::rsi, x86::rdx, x86::rcx, x86::r8, x86::r9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      // pass f64 args
      int float_arg_idx = 0;
      while (float_arg_idx < floating_point_call_args_.size() &&
             float_arg_idx < float_arg_regs.size()) {
        auto v = floating_point_call_args_[float_arg_idx];

        if (v.IsConstantGlobal()) {
          double c =
              f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          auto label = asm_->newLabel();
          asm_->section(data_section_);
          asm_->bind(label);
          asm_->embedDouble(c);
          asm_->section(text_section_);
          asm_->movsd(float_arg_regs[float_arg_idx], x86::qword_ptr(label));
        } else {
          asm_->movsd(float_arg_regs[float_arg_idx],
                      x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        }
        float_arg_idx++;
      }
      if (float_arg_idx < floating_point_call_args_.size()) {
        throw std::runtime_error("Too many floating point args for the call.");
      }

      // pass regular args
      int regular_arg_idx = 0;
      while (regular_arg_idx < qword_arg_regs.size() &&
             regular_arg_idx < regular_call_args_.size()) {
        auto [v, type] = regular_call_args_[regular_arg_idx];

        if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
          if (v.IsConstantGlobal()) {
            int8_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(byte_arg_regs[regular_arg_idx], c);
          } else {
            asm_->mov(byte_arg_regs[regular_arg_idx],
                      x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
          }
        } else if (type_manager.IsI16Type(type)) {
          if (v.IsConstantGlobal()) {
            int16_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(word_arg_regs[regular_arg_idx], c);
          } else {
            asm_->mov(word_arg_regs[regular_arg_idx],
                      x86::word_ptr(x86::rbp, offsets[v.GetIdx()]));
          }
        } else if (type_manager.IsI32Type(type)) {
          if (v.IsConstantGlobal()) {
            int32_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(dword_arg_regs[regular_arg_idx], c);
          } else {
            asm_->mov(dword_arg_regs[regular_arg_idx],
                      x86::dword_ptr(x86::rbp, offsets[v.GetIdx()]));
          }
        } else if (type_manager.IsI64Type(type)) {
          if (v.IsConstantGlobal()) {
            int64_t c = i64_constants[Type1InstructionReader(
                                          constant_instrs[v.GetIdx()])
                                          .Constant()];
            asm_->mov(qword_arg_regs[regular_arg_idx], c);
          } else {
            asm_->mov(qword_arg_regs[regular_arg_idx],
                      x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
          }
        } else if (type_manager.IsPtrType(type)) {
          int32_t ptr_offset = 0;

          if (IsGep(v, instructions)) {
            auto [ptr, o] =
                Gep(v, instructions, constant_instrs, i64_constants);
            v = ptr;
            ptr_offset = o;
          }

          if (v.IsConstantGlobal()) {
            if (ConstantOpcodeFrom(
                    GenericInstructionReader(constant_instrs[v.GetIdx()])
                        .Opcode()) == ConstantOpcode::NULLPTR) {
              asm_->mov(qword_arg_regs[regular_arg_idx], 0);
            } else {
              auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
              asm_->lea(qword_arg_regs[regular_arg_idx],
                        x86::ptr(label, ptr_offset));
            }
          } else {
            if (ptr_offset != 0) {
              asm_->mov(x86::rdx,
                        x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
              asm_->lea(qword_arg_regs[regular_arg_idx],
                        x86::ptr(x86::rdx, ptr_offset));
            } else {
              asm_->mov(qword_arg_regs[regular_arg_idx],
                        x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
            }
          }
        } else {
          throw std::runtime_error("Invalid argument type.");
        }

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
          auto [v, type] = regular_call_args_[i];

          if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
            if (v.IsConstantGlobal()) {
              int8_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                             .Constant();
              asm_->mov(x86::rax, c);
            } else {
              asm_->movzx(x86::rax,
                          x86::byte_ptr(x86::rbp, offsets[v.GetIdx()]));
            }
          } else if (type_manager.IsI16Type(type)) {
            if (v.IsConstantGlobal()) {
              int16_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant();
              asm_->mov(x86::rax, c);
            } else {
              asm_->movzx(x86::rax,
                          x86::word_ptr(x86::rbp, offsets[v.GetIdx()]));
            }
          } else if (type_manager.IsI32Type(type)) {
            if (v.IsConstantGlobal()) {
              int32_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant();
              asm_->mov(x86::rax, c);
            } else {
              asm_->movzx(x86::rax,
                          x86::dword_ptr(x86::rbp, offsets[v.GetIdx()]));
            }
          } else if (type_manager.IsI64Type(type)) {
            if (v.IsConstantGlobal()) {
              int64_t c = i64_constants[Type1InstructionReader(
                                            constant_instrs[v.GetIdx()])
                                            .Constant()];
              asm_->mov(x86::rax, c);
            } else {
              asm_->mov(x86::rax,
                        x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
            }
          } else if (type_manager.IsPtrType(type)) {
            int64_t ptr_offset = 0;

            if (IsGep(v, instructions)) {
              auto [ptr, o] =
                  Gep(v, instructions, constant_instrs, i64_constants);
              v = ptr;
              ptr_offset = o;
            }

            if (v.IsConstantGlobal()) {
              if (ConstantOpcodeFrom(
                      GenericInstructionReader(constant_instrs[v.GetIdx()])
                          .Opcode()) == ConstantOpcode::NULLPTR) {
                asm_->mov(x86::rax, 0);
              } else {
                auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
                asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
              }
            } else {
              asm_->mov(x86::rax,
                        x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
              if (ptr_offset != 0) {
                asm_->mov(x86::rdx, ptr_offset);
                asm_->add(x86::rax, x86::rdx);
              }
            }
          } else {
            throw std::runtime_error("Invalid argument type.");
          }

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
        khir::Value v(reader.Arg());
        if (v.IsConstantGlobal()) {
          throw std::runtime_error("Not possible.");
        }

        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, offsets[v.GetIdx()]));
        asm_->call(x86::rax);

        return_type = type_manager.GetFunctionReturnType(
            static_cast<Type>(reader.TypeID()));
      }

      if (dynamic_stack_alloc != 0) {
        asm_->add(x86::rsp, dynamic_stack_alloc);
      }

      if (type_manager.IsVoid(return_type)) {
        return;
      }

      auto offset = stack_allocator.AllocateSlot();
      if (type_manager.IsF64Type(return_type)) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
      } else if (type_manager.IsI1Type(return_type) ||
                 type_manager.IsI8Type(return_type)) {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      } else if (type_manager.IsI16Type(return_type)) {
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      } else if (type_manager.IsI32Type(return_type)) {
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      } else if (type_manager.IsI64Type(return_type) ||
                 type_manager.IsPtrType(return_type)) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      } else {
        throw std::runtime_error("Invalid return type.");
      }
      offsets[instr_idx] = offset;
      return;
    }
  }
}

void ASMBackend::Execute() {
  void* buffer_start;
  rt_.add(&buffer_start, &code_);

  auto offset = code_.labelOffsetFromBase(compute_label_);
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(
      reinterpret_cast<uint64_t>(buffer_start) + offset);

  comp = std::chrono::system_clock::now();

  compute();

  end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> elapsed_seconds = comp - start;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - comp;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

}  // namespace kush::khir