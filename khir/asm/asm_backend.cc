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
    const auto& instructions = func.Instructions();
    const auto& basic_blocks = func.BasicBlocks();

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

    std::cerr << func.Name() << std::endl;
    auto [live_intervals, order] = ComputeLiveIntervals(func, type_manager);
    auto register_assign =
        AssignRegisters(live_intervals, instructions, type_manager);

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

    std::vector<Label> basic_blocks_impl;
    Label epilogue = asm_->newLabel();
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(asm_->newLabel());
    }

    std::vector<int32_t> value_offsets(instructions.size(), INT32_MAX);

    // initially, after rbp is all callee saved registers
    StackSlotAllocator static_stack_allocator(8 *
                                              callee_saved_registers.size());
    for (int i : order) {
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

int32_t Get(std::vector<int32_t>& offsets, int i) {
  auto offset = offsets[i];
  if (offset == INT32_MAX) {
    throw std::runtime_error("Undefined offset.");
  }

  return offset;
}

void ASMBackend::TranslateInstr(
    const TypeManager& type_manager, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<Label>& basic_blocks,
    const std::vector<Function>& functions, const Label& epilogue,
    std::vector<int32_t>& offsets, const std::vector<uint64_t>& instructions,
    const std::vector<uint64_t>& constant_instrs, int instr_idx,
    StackSlotAllocator& stack_allocator,
    const std::vector<RegisterAssignment>& register_assign) {
  auto instr = instructions[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  /*
  Available for allocation:
    RBX, RCX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6

  Reserved/Scratch
    RSP, RBP, RAX, R10, XMM7
  */
  const std::vector<Register> normal_registers{
      Register::RBX, Register::RCX, Register::RDX, Register::RSI,
      Register::RDI, Register::R8,  Register::R9,  Register::R11,
      Register::R12, Register::R13, Register::R14, Register::R15};
  const std::vector<x86::Xmm> fp_registers{
      x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3,
      x86::xmm4, x86::xmm5, x86::xmm6,
  };

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
      int32_t offset = INT32_MAX;
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
                  (x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx()))));
      } else {  // v on stack, dest on stack
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
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
      int32_t offset = INT32_MAX;
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
                  (x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx()))));
        asm_->xor_(normal_registers[dest_reg].GetB(), 1);
      } else {  // v on stack, dest on stack
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
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

      int32_t offset = INT32_MAX;
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
        asm_->mov(dest, x86::byte_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->add(dest, c1);
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetB());
      } else {
        asm_->add(dest, x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
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

      int32_t offset = INT32_MAX;
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
        asm_->mov(dest, x86::byte_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->sub(dest, c1);
      } else if (v1_is_reg) {
        asm_->sub(dest, normal_registers[v1_reg].GetB());
      } else {
        asm_->sub(dest, x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
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

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        asm_->mov(x86::ax, c0);
      } else if (v0_is_reg) {
        asm_->movzx(x86::ax, normal_registers[v0_reg].GetB());
      } else {
        asm_->movzx(x86::ax,
                    x86::byte_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->imul(x86::ax, x86::ax, c1);
      } else if (v1_is_reg) {
        asm_->imul(normal_registers[v1_reg].GetB());
      } else {
        asm_->imul(x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(), x86::al);
      } else {
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
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

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::GpbLo arg0;
      if (v0.IsConstantGlobal()) {
        arg0 = x86::al;
        asm_->mov(x86::eax, c0);
      } else if (v0_is_reg) {
        arg0 = normal_registers[v0_reg].GetB();
      } else {
        arg0 = x86::al;
        asm_->movzx(x86::eax,
                    x86::byte_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->cmp(arg0, c1);
      } else if (v1_is_reg) {
        asm_->cmp(arg0, normal_registers[v1_reg].GetB());
      } else {
        asm_->cmp(arg0, x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      switch (opcode) {
        case Opcode::I1_CMP_EQ:
        case Opcode::I8_CMP_EQ: {
          if (dest_is_reg) {
            asm_->sete(normal_registers[dest_reg].GetB());
          } else {
            asm_->sete(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I1_CMP_NE:
        case Opcode::I8_CMP_NE: {
          if (dest_is_reg) {
            asm_->setne(normal_registers[dest_reg].GetB());
          } else {
            asm_->setne(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I8_CMP_LT: {
          if (dest_is_reg) {
            asm_->setl(normal_registers[dest_reg].GetB());
          } else {
            asm_->setl(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I8_CMP_LE: {
          if (dest_is_reg) {
            asm_->setle(normal_registers[dest_reg].GetB());
          } else {
            asm_->setle(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I8_CMP_GT: {
          if (dest_is_reg) {
            asm_->setg(normal_registers[dest_reg].GetB());
          } else {
            asm_->setg(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I8_CMP_GE: {
          if (dest_is_reg) {
            asm_->setge(normal_registers[dest_reg].GetB());
          } else {
            asm_->setge(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        default:
          throw std::runtime_error("Not possible");
      }
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      uint8_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      uint32_t constant = c;
      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;
      if (v.IsConstantGlobal()) {
        asm_->mov(dest, constant);
      } else if (v_is_reg) {
        asm_->movzx(dest, normal_registers[v_reg].GetB());
      } else {
        asm_->movzx(dest, x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I8_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int8_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      double constant = c;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(constant);
        asm_->section(text_section_);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (v_is_reg) {
        asm_->movsx(x86::eax, normal_registers[v_reg].GetB());
        asm_->cvtsi2sd(dest, x86::eax);
      } else {
        asm_->movsx(x86::eax,
                    x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        asm_->cvtsi2sd(dest, x86::eax);
      }

      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int8_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (v1.IsConstantGlobal()) {
          asm_->mov(x86::byte_ptr(label, ptr_offset), c1);
        } else if (v1_is_reg) {
          asm_->mov(x86::byte_ptr(label, ptr_offset),
                    normal_registers[v1_reg].GetB());
        } else {
          asm_->mov(x86::al,
                    x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::byte_ptr(label, ptr_offset), x86::al);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->mov(x86::byte_ptr(ptr_reg, ptr_offset), c1);
      } else if (v1_is_reg) {
        asm_->mov(x86::byte_ptr(ptr_reg, ptr_offset),
                  normal_registers[v1_reg].GetB());
      } else {
        asm_->mov(x86::al, x86::byte_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::byte_ptr(ptr_reg, ptr_offset), x86::al);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->mov(normal_registers[dest_reg].GetB(),
                    x86::byte_ptr(label, ptr_offset));
        } else {
          asm_->mov(x86::al, x86::byte_ptr(label, ptr_offset));
          asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetB(),
                  x86::byte_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->mov(x86::al, x86::byte_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
      }
      return;
    }

    // I16 =====================================================================
    case Opcode::I16_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int16_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int16_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetW() : x86::ax;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetW());
      } else {
        asm_->mov(dest, x86::word_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->add(dest, c1);
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetW());
      } else {
        asm_->add(dest, x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      return;
    }

    case Opcode::I16_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int16_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int16_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetW() : x86::ax;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetW());
      } else {
        asm_->mov(dest, x86::word_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->sub(dest, c1);
      } else if (v1_is_reg) {
        asm_->sub(dest, normal_registers[v1_reg].GetW());
      } else {
        asm_->sub(dest, x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
    }

    case Opcode::I16_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int16_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int16_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        asm_->mov(x86::eax, c0);
      } else if (v0_is_reg) {
        asm_->movzx(x86::eax, normal_registers[v0_reg].GetW());
      } else {
        asm_->movzx(x86::eax,
                    x86::word_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->imul(x86::ax, x86::ax, c1);
      } else if (v1_is_reg) {
        asm_->imul(normal_registers[v1_reg].GetW());
      } else {
        asm_->imul(x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetW(), x86::ax);
      } else {
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      return;
    }

    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int16_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int16_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Gpw arg0;
      if (v0.IsConstantGlobal()) {
        arg0 = x86::ax;
        asm_->mov(x86::eax, c0);
      } else if (v0_is_reg) {
        arg0 = normal_registers[v0_reg].GetW();
      } else {
        arg0 = x86::ax;
        asm_->movzx(x86::eax,
                    x86::word_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->cmp(arg0, c1);
      } else if (v1_is_reg) {
        asm_->cmp(arg0, normal_registers[v1_reg].GetW());
      } else {
        asm_->cmp(arg0, x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      switch (opcode) {
        case Opcode::I16_CMP_EQ: {
          if (dest_is_reg) {
            asm_->sete(normal_registers[dest_reg].GetB());
          } else {
            asm_->sete(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I16_CMP_NE: {
          if (dest_is_reg) {
            asm_->setne(normal_registers[dest_reg].GetB());
          } else {
            asm_->setne(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I16_CMP_LT: {
          if (dest_is_reg) {
            asm_->setl(normal_registers[dest_reg].GetB());
          } else {
            asm_->setl(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I16_CMP_LE: {
          if (dest_is_reg) {
            asm_->setle(normal_registers[dest_reg].GetB());
          } else {
            asm_->setle(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I16_CMP_GT: {
          if (dest_is_reg) {
            asm_->setg(normal_registers[dest_reg].GetB());
          } else {
            asm_->setg(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::I16_CMP_GE: {
          if (dest_is_reg) {
            asm_->setge(normal_registers[dest_reg].GetB());
          } else {
            asm_->setge(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        default:
          throw std::runtime_error("Not possible");
      }
    }

    case Opcode::I16_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      uint16_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      uint32_t constant = c;
      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;
      if (v.IsConstantGlobal()) {
        asm_->mov(dest, constant);
      } else if (v_is_reg) {
        asm_->movzx(dest, normal_registers[v_reg].GetW());
      } else {
        asm_->movzx(dest, x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I16_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int16_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      double constant = c;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(constant);
        asm_->section(text_section_);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (v_is_reg) {
        asm_->movsx(x86::eax, normal_registers[v_reg].GetW());
        asm_->cvtsi2sd(dest, x86::eax);
      } else {
        asm_->movsx(x86::eax,
                    x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        asm_->cvtsi2sd(dest, x86::eax);
      }

      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int16_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (v1.IsConstantGlobal()) {
          asm_->mov(x86::word_ptr(label, ptr_offset), c1);
        } else if (v1_is_reg) {
          asm_->mov(x86::word_ptr(label, ptr_offset),
                    normal_registers[v1_reg].GetW());
        } else {
          asm_->movzx(x86::eax,
                      x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::word_ptr(label, ptr_offset), x86::ax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->mov(x86::word_ptr(ptr_reg, ptr_offset), c1);
      } else if (v1_is_reg) {
        asm_->mov(x86::word_ptr(ptr_reg, ptr_offset),
                  normal_registers[v1_reg].GetW());
      } else {
        asm_->movzx(x86::eax,
                    x86::word_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::word_ptr(ptr_reg, ptr_offset), x86::eax);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->mov(normal_registers[dest_reg].GetW(),
                    x86::word_ptr(label, ptr_offset));
        } else {
          asm_->movzx(x86::eax, x86::word_ptr(label, ptr_offset));
          asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetW(),
                  x86::word_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->movzx(x86::eax, x86::word_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
      }
      return;
    }

    // I32 =====================================================================
    case Opcode::I32_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int32_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int32_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetD() : x86::eax;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetD());
      } else {
        asm_->mov(dest, x86::dword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->add(dest, c1);
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetD());
      } else {
        asm_->add(dest, x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      return;
    }

    case Opcode::I32_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int32_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int32_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetD() : x86::eax;

      if (v0.IsConstantGlobal()) {
        asm_->sub(dest, c0);
      } else if (v0_is_reg) {
        asm_->sub(dest, normal_registers[v0_reg].GetD());
      } else {
        asm_->sub(dest, x86::dword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->add(dest, c1);
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetD());
      } else {
        asm_->add(dest, x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      return;
    }

    case Opcode::I32_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int32_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int32_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        asm_->mov(x86::eax, c0);
      } else if (v0_is_reg) {
        asm_->mov(x86::eax, normal_registers[v0_reg].GetD());
      } else {
        asm_->mov(x86::eax,
                  x86::dword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->imul(x86::eax, x86::eax, c1);
      } else if (v1_is_reg) {
        asm_->imul(normal_registers[v1_reg].GetD());
      } else {
        asm_->imul(x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetD(), x86::eax);
      } else {
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      return;
    }

    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int32_t c0 =
          v0.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v0.GetIdx()]).Constant()
              : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int32_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Gpd arg0;
      if (v0.IsConstantGlobal()) {
        arg0 = x86::eax;
        asm_->mov(x86::eax, c0);
      } else if (v0_is_reg) {
        arg0 = normal_registers[v0_reg].GetD();
      } else {
        arg0 = x86::eax;
        asm_->mov(x86::eax,
                  x86::dword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->cmp(arg0, c1);
      } else if (v1_is_reg) {
        asm_->cmp(arg0, normal_registers[v1_reg].GetD());
      } else {
        asm_->cmp(arg0, x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      switch (opcode) {
        case Opcode::I32_CMP_EQ: {
          if (dest_is_reg) {
            asm_->sete(normal_registers[dest_reg].GetB());
          } else {
            asm_->sete(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I32_CMP_NE: {
          if (dest_is_reg) {
            asm_->setne(normal_registers[dest_reg].GetB());
          } else {
            asm_->setne(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I32_CMP_LT: {
          if (dest_is_reg) {
            asm_->setl(normal_registers[dest_reg].GetB());
          } else {
            asm_->setl(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I32_CMP_LE: {
          if (dest_is_reg) {
            asm_->setle(normal_registers[dest_reg].GetB());
          } else {
            asm_->setle(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I32_CMP_GT: {
          if (dest_is_reg) {
            asm_->setg(normal_registers[dest_reg].GetB());
          } else {
            asm_->setg(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I32_CMP_GE: {
          if (dest_is_reg) {
            asm_->setge(normal_registers[dest_reg].GetB());
          } else {
            asm_->setge(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        default:
          throw std::runtime_error("Not possible");
      }
    }

    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      uint32_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      uint64_t constant = c;
      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;
      if (v.IsConstantGlobal()) {
        asm_->mov(dest, constant);
      } else if (v_is_reg) {
        asm_->movzx(dest, normal_registers[v_reg].GetD());
      } else {
        asm_->movzx(dest, x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I32_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int32_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;
      double constant = c;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(constant);
        asm_->section(text_section_);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (v_is_reg) {
        asm_->cvtsi2sd(dest, normal_registers[v_reg].GetD());
      } else {
        asm_->cvtsi2sd(dest,
                       x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int32_t c1 =
          v1.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant()
              : 0;

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (v1.IsConstantGlobal()) {
          asm_->mov(x86::dword_ptr(label, ptr_offset), c1);
        } else if (v1_is_reg) {
          asm_->mov(x86::dword_ptr(label, ptr_offset),
                    normal_registers[v1_reg].GetD());
        } else {
          asm_->mov(x86::eax,
                    x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::dword_ptr(label, ptr_offset), x86::eax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->mov(x86::dword_ptr(ptr_reg, ptr_offset), c1);
      } else if (v1_is_reg) {
        asm_->mov(x86::dword_ptr(ptr_reg, ptr_offset),
                  normal_registers[v1_reg].GetD());
      } else {
        asm_->mov(x86::eax,
                  x86::dword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::dword_ptr(ptr_reg, ptr_offset), x86::eax);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->mov(normal_registers[dest_reg].GetD(),
                    x86::dword_ptr(label, ptr_offset));
        } else {
          asm_->mov(x86::eax, x86::dword_ptr(label, ptr_offset));
          asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetD(),
                  x86::dword_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->mov(x86::eax, x86::dword_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
      }
      return;
    }

    // I64 =====================================================================
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int64_t c0 = v0.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v0.GetIdx()])
                                           .Constant()]
                       : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int64_t c1 = v1.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v1.GetIdx()])
                                           .Constant()]
                       : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetQ());
      } else {
        asm_->mov(dest, x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        int64_t max = INT32_MAX;
        int64_t min = INT32_MIN;
        if (c1 >= min && c1 <= max) {
          int32_t c = c1;
          asm_->add(dest, c);
        } else {
          asm_->mov(x86::r10, c1);
          asm_->add(dest, x86::r10);
        }
      } else if (v1_is_reg) {
        asm_->add(dest, normal_registers[v1_reg].GetQ());
      } else {
        asm_->add(dest, x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int64_t c0 = v0.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v0.GetIdx()])
                                           .Constant()]
                       : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int64_t c1 = v1.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v1.GetIdx()])
                                           .Constant()]
                       : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;

      if (v0.IsConstantGlobal()) {
        asm_->mov(dest, c0);
      } else if (v0_is_reg) {
        asm_->mov(dest, normal_registers[v0_reg].GetQ());
      } else {
        asm_->mov(dest, x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        int64_t max = INT32_MAX;
        int64_t min = INT32_MIN;
        if (c1 >= min && c1 <= max) {
          int32_t c = c1;
          asm_->sub(dest, c);
        } else {
          asm_->mov(x86::r10, c1);
          asm_->sub(dest, x86::r10);
        }
      } else if (v1_is_reg) {
        asm_->sub(dest, normal_registers[v1_reg].GetQ());
      } else {
        asm_->sub(dest, x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int64_t c0 = v0.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v0.GetIdx()])
                                           .Constant()]
                       : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int64_t c1 = v1.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v1.GetIdx()])
                                           .Constant()]
                       : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        asm_->mov(x86::rax, c0);
      } else if (v0_is_reg) {
        asm_->mov(x86::rax, normal_registers[v0_reg].GetQ());
      } else {
        asm_->mov(x86::rax,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        int64_t max = INT32_MAX;
        int64_t min = INT32_MIN;
        if (c1 >= min && c1 <= max) {
          int32_t c = c1;
          asm_->imul(x86::rax, x86::rax, c);
        } else {
          asm_->mov(x86::r10, c1);
          asm_->imul(x86::r10);
        }
      } else if (v1_is_reg) {
        asm_->imul(normal_registers[v1_reg].GetQ());
      } else {
        asm_->imul(x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetQ(), x86::rax);
      } else {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      int64_t c0 = v0.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v0.GetIdx()])
                                           .Constant()]
                       : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int64_t c1 = v1.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v1.GetIdx()])
                                           .Constant()]
                       : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Gpq arg0;
      if (v0.IsConstantGlobal()) {
        arg0 = x86::rax;
        asm_->mov(x86::rax, c0);
      } else if (v0_is_reg) {
        arg0 = normal_registers[v0_reg].GetQ();
      } else {
        arg0 = x86::rax;
        asm_->mov(x86::rax,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        int64_t max = INT32_MAX;
        int64_t min = INT32_MIN;
        if (c1 >= min && c1 <= max) {
          int32_t c = c1;
          asm_->cmp(x86::rax, c);
        } else {
          asm_->mov(x86::r10, c1);
          asm_->cmp(x86::rax, x86::r10);
        }
      } else if (v1_is_reg) {
        asm_->cmp(arg0, normal_registers[v1_reg].GetQ());
      } else {
        asm_->cmp(arg0, x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      switch (opcode) {
        case Opcode::I64_CMP_EQ: {
          if (dest_is_reg) {
            asm_->sete(normal_registers[dest_reg].GetB());
          } else {
            asm_->sete(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I64_CMP_NE: {
          if (dest_is_reg) {
            asm_->setne(normal_registers[dest_reg].GetB());
          } else {
            asm_->setne(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I64_CMP_LT: {
          if (dest_is_reg) {
            asm_->setl(normal_registers[dest_reg].GetB());
          } else {
            asm_->setl(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I64_CMP_LE: {
          if (dest_is_reg) {
            asm_->setle(normal_registers[dest_reg].GetB());
          } else {
            asm_->setle(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I64_CMP_GT: {
          if (dest_is_reg) {
            asm_->setg(normal_registers[dest_reg].GetB());
          } else {
            asm_->setg(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        case Opcode::I64_CMP_GE: {
          if (dest_is_reg) {
            asm_->setge(normal_registers[dest_reg].GetB());
          } else {
            asm_->setge(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        default:
          throw std::runtime_error("Not possible");
      }
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int64_t c = v.IsConstantGlobal()
                      ? i64_constants[Type1InstructionReader(
                                          constant_instrs[v.GetIdx()])
                                          .Constant()]
                      : 0;
      double constant = c;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(constant);
        asm_->section(text_section_);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (v_is_reg) {
        asm_->cvtsi2sd(dest, normal_registers[v_reg].GetQ());
      } else {
        asm_->cvtsi2sd(dest,
                       x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      int64_t c1 = v1.IsConstantGlobal()
                       ? i64_constants[Type1InstructionReader(
                                           constant_instrs[v1.GetIdx()])
                                           .Constant()]
                       : 0;

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (v1.IsConstantGlobal()) {
          int64_t max = INT32_MAX;
          int64_t min = INT32_MIN;
          if (c1 >= min && c1 <= max) {
            int32_t c = c1;
            asm_->mov(x86::qword_ptr(label, ptr_offset), c);
          } else {
            asm_->mov(x86::rax, c1);
            asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
          }
        } else if (v1_is_reg) {
          asm_->mov(x86::qword_ptr(label, ptr_offset),
                    normal_registers[v1_reg].GetQ());
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        int64_t max = INT32_MAX;
        int64_t min = INT32_MIN;
        if (c1 >= min && c1 <= max) {
          int32_t c = c1;
          asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), c);
        } else {
          asm_->mov(x86::rax, c1);
          asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
        }
      } else if (v1_is_reg) {
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset),
                  normal_registers[v1_reg].GetQ());
      } else {
        asm_->mov(x86::rax,
                  x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->mov(normal_registers[dest_reg].GetQ(),
                    x86::qword_ptr(label, ptr_offset));
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetQ(),
                  x86::qword_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
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

      auto ptr_type = static_cast<khir::Type>(reader.TypeID());
      auto type = type_manager.GetPointerElementType(ptr_type);
      auto size = type_manager.GetTypeSize(type);
      size += (16 - (size % 16)) % 16;
      assert(size % 16 == 0);

      asm_->sub(x86::rsp, size);

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetQ(), x86::rsp);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rsp);
      }
      return;
    }

    case Opcode::PTR_MATERIALIZE: {
      throw std::runtime_error("Unimplemented");
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

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;
      if (v.IsConstantGlobal()) {
        if (ConstantOpcodeFrom(
                GenericInstructionReader(constant_instrs[v.GetIdx()])
                    .Opcode()) == ConstantOpcode::NULLPTR) {
          asm_->mov(dest, ptr_offset);
        } else {
          auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
          asm_->lea(dest, x86::ptr(label, ptr_offset));
        }
      } else if (v_is_reg) {
        if (ptr_offset != 0) {
          asm_->lea(dest, x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
        } else {
          asm_->mov(dest, normal_registers[v_reg].GetQ());
        }
      } else {
        if (ptr_offset != 0) {
          asm_->mov(x86::r10,
                    x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          asm_->lea(dest, x86::ptr(x86::r10, ptr_offset));
        } else {
          asm_->mov(dest, x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;

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
        } else if (v1_is_reg) {
          asm_->mov(x86::qword_ptr(label, ptr_offset),
                    normal_registers[v1_reg].GetQ());
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        if (ConstantOpcodeFrom(
                GenericInstructionReader(constant_instrs[v1.GetIdx()])
                    .Opcode()) == ConstantOpcode::NULLPTR) {
          asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), 0);
        } else {
          auto label = GetConstantGlobal(constant_instrs[v1.GetIdx()]);
          asm_->lea(x86::rax, x86::ptr(label));
          asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
        }
      } else if (v1_is_reg) {
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset),
                  normal_registers[v1_reg].GetQ());
      } else {
        asm_->mov(x86::rax,
                  x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->mov(normal_registers[dest_reg].GetQ(),
                    x86::qword_ptr(label, ptr_offset));
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->mov(normal_registers[dest_reg].GetQ(),
                  x86::qword_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
      return;
    }

    // F64 =====================================================================
    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Xmm arg0;
      if (v0.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c0);
        asm_->section(text_section_);
        asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        arg0 = x86::xmm7;
      } else if (v0_is_reg) {
        arg0 = fp_registers[v0_reg];
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
        arg0 = x86::xmm7;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v1.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c1);
        asm_->section(text_section_);
        asm_->vaddsd(dest, arg0, x86::qword_ptr(label));
      } else if (v1_is_reg) {
        asm_->vaddsd(dest, arg0, fp_registers[v1_reg]);
      } else {
        asm_->vaddsd(dest, arg0,
                     x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }
      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Xmm arg0;
      if (v0.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c0);
        asm_->section(text_section_);
        asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        arg0 = x86::xmm7;
      } else if (v0_is_reg) {
        arg0 = fp_registers[v0_reg];
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
        arg0 = x86::xmm7;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v1.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c1);
        asm_->section(text_section_);
        asm_->vsubsd(dest, arg0, x86::qword_ptr(label));
      } else if (v1_is_reg) {
        asm_->vsubsd(dest, arg0, fp_registers[v1_reg]);
      } else {
        asm_->vsubsd(dest, arg0,
                     x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }
      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Xmm arg0;
      if (v0.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c0);
        asm_->section(text_section_);
        asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        arg0 = x86::xmm7;
      } else if (v0_is_reg) {
        arg0 = fp_registers[v0_reg];
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
        arg0 = x86::xmm7;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v1.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c1);
        asm_->section(text_section_);
        asm_->vmulsd(dest, arg0, x86::qword_ptr(label));
      } else if (v1_is_reg) {
        asm_->vmulsd(dest, arg0, fp_registers[v1_reg]);
      } else {
        asm_->vmulsd(dest, arg0,
                     x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }
      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Xmm arg0;
      if (v0.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c0);
        asm_->section(text_section_);
        asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        arg0 = x86::xmm7;
      } else if (v0_is_reg) {
        arg0 = fp_registers[v0_reg];
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
        arg0 = x86::xmm7;
      }

      auto dest = dest_is_reg ? fp_registers[dest_reg] : x86::xmm7;
      if (v1.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c1);
        asm_->section(text_section_);
        asm_->vdivsd(dest, arg0, x86::qword_ptr(label));
      } else if (v1_is_reg) {
        asm_->vdivsd(dest, arg0, fp_registers[v1_reg]);
      } else {
        asm_->vdivsd(dest, arg0,
                     x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }
      if (!dest_is_reg) {
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (opcode == Opcode::F64_CMP_LT || opcode == Opcode::F64_CMP_LE) {
        std::swap(v0, v1);
      }

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      x86::Xmm arg0;
      if (v0.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c0);
        asm_->section(text_section_);
        asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        arg0 = x86::xmm7;
      } else if (v0_is_reg) {
        arg0 = fp_registers[v0_reg];
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
        arg0 = x86::xmm7;
      }

      if (v1.IsConstantGlobal()) {
        auto label = asm_->newLabel();
        asm_->section(data_section_);
        asm_->bind(label);
        asm_->embedDouble(c1);
        asm_->section(text_section_);
        asm_->vucomisd(arg0, x86::qword_ptr(label));
      } else if (v1_is_reg) {
        asm_->vucomisd(arg0, fp_registers[v1_reg]);
      } else {
        asm_->vucomisd(arg0,
                       x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
      }

      switch (opcode) {
        case Opcode::F64_CMP_EQ: {
          auto dest = dest_is_reg ? normal_registers[dest_reg].GetB() : x86::al;
          asm_->setnp(dest);
          asm_->sete(x86::dl);
          asm_->and_(dest, x86::dl);
          if (!dest_is_reg) {
            asm_->mov(x86::byte_ptr(x86::rbp, offset), dest);
          }
          return;
        }

        case Opcode::F64_CMP_NE: {
          auto dest = dest_is_reg ? normal_registers[dest_reg].GetB() : x86::al;
          asm_->setnp(dest);
          asm_->setne(x86::dl);
          asm_->or_(dest, x86::dl);
          if (!dest_is_reg) {
            asm_->mov(x86::byte_ptr(x86::rbp, offset), dest);
          }
          return;
        }

        case Opcode::F64_CMP_LT:
        case Opcode::F64_CMP_GT: {
          if (dest_is_reg) {
            asm_->seta(normal_registers[dest_reg].GetB());
          } else {
            asm_->seta(x86::byte_ptr(x86::rbp, offset));
          }

          return;
        }

        case Opcode::F64_CMP_LE:
        case Opcode::F64_CMP_GE: {
          if (dest_is_reg) {
            asm_->setae(normal_registers[dest_reg].GetB());
          } else {
            asm_->setae(x86::byte_ptr(x86::rbp, offset));
          }
          return;
        }

        default:
          throw std::runtime_error("Impossible");
      }
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;
      double c0 = v0.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v0.GetIdx()])
                                          .Constant()]
                      : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      auto dest = dest_is_reg ? normal_registers[dest_reg].GetQ() : x86::rax;
      if (v0.IsConstantGlobal()) {
        int64_t conv = c0;
        asm_->mov(dest, conv);
      } else if (v0_is_reg) {
        asm_->cvttsd2si(dest, fp_registers[v0_reg]);
      } else {
        asm_->cvttsd2si(dest,
                        x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (!dest_is_reg) {
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      bool v1_is_reg =
          !v1.IsConstantGlobal() && register_assign[v1.GetIdx()] >= 0;
      int v1_reg = v1_is_reg ? register_assign[v1.GetIdx()] : 0;
      double c1 = v1.IsConstantGlobal()
                      ? f64_constants[Type1InstructionReader(
                                          constant_instrs[v1.GetIdx()])
                                          .Constant()]
                      : 0;
      uint64_t c1_as_int;
      std::memcpy(&c1_as_int, &c1, sizeof(c1_as_int));

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (v1.IsConstantGlobal()) {
          asm_->mov(x86::rax, c1_as_int);
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        } else if (v1_is_reg) {
          asm_->movsd(x86::qword_ptr(label, ptr_offset), fp_registers[v1_reg]);
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
          asm_->mov(x86::qword_ptr(label, ptr_offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (v1.IsConstantGlobal()) {
        asm_->mov(x86::rax, c1_as_int);
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
      } else if (v1_is_reg) {
        asm_->movsd(x86::qword_ptr(ptr_reg, ptr_offset), fp_registers[v1_reg]);
      } else {
        asm_->mov(x86::rax,
                  x86::qword_ptr(x86::rbp, Get(offsets, v1.GetIdx())));
        asm_->mov(x86::qword_ptr(ptr_reg, ptr_offset), x86::rax);
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

      bool v0_is_reg =
          !v0.IsConstantGlobal() && register_assign[v0.GetIdx()] >= 0;
      int v0_reg = v0_is_reg ? register_assign[v0.GetIdx()] : 0;

      int32_t offset = INT32_MAX;
      if (!dest_is_reg) {
        offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
      }

      if (v0.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        if (dest_is_reg) {
          asm_->movsd(fp_registers[dest_reg],
                      x86::qword_ptr(label, ptr_offset));
        } else {
          asm_->mov(x86::rax, x86::qword_ptr(label, ptr_offset));
          asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
        }
        return;
      }

      auto ptr_reg = v0_is_reg ? normal_registers[v0_reg].GetQ() : x86::r10;
      if (!v0_is_reg) {
        asm_->mov(x86::r10,
                  x86::qword_ptr(x86::rbp, Get(offsets, v0.GetIdx())));
      }

      if (dest_is_reg) {
        asm_->movsd(fp_registers[dest_reg],
                    x86::qword_ptr(ptr_reg, ptr_offset));
      } else {
        asm_->mov(x86::rax, x86::qword_ptr(ptr_reg, ptr_offset));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
      }
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

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;
      int8_t c =
          v.IsConstantGlobal()
              ? Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant()
              : 0;

      if (v.IsConstantGlobal()) {
        if (c != 0) {
          asm_->jmp(basic_blocks[reader.Marg0()]);
        } else {
          asm_->jmp(basic_blocks[reader.Marg1()]);
        }
      } else if (v_is_reg) {
        asm_->cmp(normal_registers[v_reg].GetB(), 1);
        asm_->je(basic_blocks[reader.Marg0()]);
        asm_->jmp(basic_blocks[reader.Marg1()]);
      } else {
        asm_->cmp(x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())), 1);
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

      bool dest_is_reg = register_assign[phi.GetIdx()] >= 0;
      int dest_reg = register_assign[phi.GetIdx()];

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

      int32_t offset = offsets[phi.GetIdx()];
      if (!dest_is_reg && offset == INT32_MAX) {
        offset = stack_allocator.AllocateSlot();
        offsets[phi.GetIdx()] = offset;
      }

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

          if (dest_is_reg) {
            asm_->movsd(fp_registers[dest_reg], x86::qword_ptr(label));
          } else {
            asm_->movsd(x86::xmm7, x86::qword_ptr(label));
            asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->movsd(fp_registers[dest_reg], fp_registers[v_reg]);
          } else {
            asm_->movsd(x86::qword_ptr(x86::rbp, offset), fp_registers[v_reg]);
          }
        } else {
          if (dest_is_reg) {
            asm_->movsd(fp_registers[dest_reg],
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          } else {
            asm_->movsd(x86::xmm7,
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
          }
        }
        return;
      }

      if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (v.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetB(), c);
          } else {
            asm_->mov(x86::byte_ptr(x86::rbp, offset), c);
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetB(),
                      normal_registers[v_reg].GetB());
          } else {
            asm_->mov(x86::byte_ptr(x86::rbp, offset),
                      normal_registers[v_reg].GetB());
          }
        } else {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetB(),
                      x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          } else {
            asm_->mov(x86::al,
                      x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            asm_->mov(x86::byte_ptr(x86::rbp, offset), x86::al);
          }
        }
        return;
      }

      if (type_manager.IsI16Type(type)) {
        if (v.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetW(), c);
          } else {
            asm_->mov(x86::word_ptr(x86::rbp, offset), c);
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetW(),
                      normal_registers[v_reg].GetW());
          } else {
            asm_->mov(x86::word_ptr(x86::rbp, offset),
                      normal_registers[v_reg].GetW());
          }
        } else {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetW(),
                      x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          } else {
            asm_->mov(x86::ax,
                      x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            asm_->mov(x86::word_ptr(x86::rbp, offset), x86::ax);
          }
        }
        return;
      }

      if (type_manager.IsI32Type(type)) {
        if (v.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetD(), c);
          } else {
            asm_->mov(x86::dword_ptr(x86::rbp, offset), c);
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetD(),
                      normal_registers[v_reg].GetD());
          } else {
            asm_->mov(x86::dword_ptr(x86::rbp, offset),
                      normal_registers[v_reg].GetD());
          }
        } else {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetD(),
                      x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          } else {
            asm_->mov(x86::eax,
                      x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            asm_->mov(x86::dword_ptr(x86::rbp, offset), x86::eax);
          }
        }
        return;
      }

      if (type_manager.IsI64Type(type)) {
        if (v.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];

          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetQ(), c);
          } else {
            int64_t max = INT32_MAX;
            int64_t min = INT32_MIN;
            if (c >= min && c <= max) {
              int32_t c32 = c;
              asm_->mov(x86::qword_ptr(x86::rbp, offset), c32);
            } else {
              asm_->mov(x86::rax, c);
              asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
            }
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetQ(),
                      normal_registers[v_reg].GetQ());
          } else {
            asm_->mov(x86::qword_ptr(x86::rbp, offset),
                      normal_registers[v_reg].GetQ());
          }
        } else {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetQ(),
                      x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          } else {
            asm_->mov(x86::rax,
                      x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
          }
        }
        return;
      }

      if (type_manager.IsPtrType(type)) {
        int32_t ptr_offset = 0;

        if (IsGep(v, instructions)) {
          auto [ptr, o] = Gep(v, instructions, constant_instrs, i64_constants);
          v = ptr;
          ptr_offset = o;
        }

        bool v_is_reg =
            !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
        int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

        if (v.IsConstantGlobal()) {
          if (ConstantOpcodeFrom(
                  GenericInstructionReader(constant_instrs[v.GetIdx()])
                      .Opcode()) == ConstantOpcode::NULLPTR) {
            if (dest_is_reg) {
              asm_->mov(normal_registers[dest_reg].GetQ(), ptr_offset);
            } else {
              asm_->mov(x86::qword_ptr(x86::rbp, offset), ptr_offset);
            }
          } else {
            auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);

            if (dest_is_reg) {
              asm_->lea(normal_registers[dest_reg].GetQ(),
                        x86::ptr(label, ptr_offset));
            } else {
              asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
              asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
            }
          }
        } else if (v_is_reg) {
          if (dest_is_reg) {
            asm_->lea(normal_registers[dest_reg].GetQ(),
                      x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
          } else {
            asm_->lea(x86::rax,
                      x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
          }
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));

          if (dest_is_reg) {
            asm_->lea(normal_registers[dest_reg].GetQ(),
                      x86::ptr(x86::rax, ptr_offset));
          } else {
            asm_->lea(x86::rax, x86::ptr(x86::rax, ptr_offset));
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
          }
        }
        return;
      }

      throw std::runtime_error("Invalid argument type.");
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

      bool v_is_reg = !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
      int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

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
          asm_->movsd(x86::xmm7, x86::qword_ptr(label));
        } else if (v_is_reg) {
          asm_->movsd(x86::xmm7, fp_registers[v.GetIdx()]);
        } else {
          asm_->movsd(x86::xmm7,
                      x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        return;
      }

      if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (v.IsConstantGlobal()) {
          int8_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::al, c);
        } else if (v_is_reg) {
          asm_->mov(x86::al, normal_registers[v_reg].GetB());
        } else {
          asm_->mov(x86::al, x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        return;
      }

      if (type_manager.IsI16Type(type)) {
        if (v.IsConstantGlobal()) {
          int16_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::ax, c);
        } else if (v_is_reg) {
          asm_->mov(x86::ax, normal_registers[v_reg].GetW());
        } else {
          asm_->mov(x86::ax, x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        return;
      }

      if (type_manager.IsI32Type(type)) {
        if (v.IsConstantGlobal()) {
          int32_t c =
              Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
          asm_->mov(x86::eax, c);
        } else if (v_is_reg) {
          asm_->mov(x86::eax, normal_registers[v_reg].GetD());
        } else {
          asm_->mov(x86::eax,
                    x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        return;
      }

      if (type_manager.IsI64Type(type)) {
        if (v.IsConstantGlobal()) {
          int64_t c =
              i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                                .Constant()];
          asm_->mov(x86::rax, c);
        } else if (v_is_reg) {
          asm_->mov(x86::rax, normal_registers[v_reg].GetQ());
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        return;
      }

      if (type_manager.IsPtrType(type)) {
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
            asm_->mov(x86::rax, ptr_offset);
          } else {
            auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
            asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
          }
        } else if (v_is_reg) {
          asm_->lea(x86::rax,
                    x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
        } else {
          asm_->mov(x86::rax,
                    x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          asm_->lea(x86::rax, x86::ptr(x86::rax, ptr_offset));
        }
      }

      asm_->jmp(epilogue);
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      const std::vector<Register> normal_arg_regs = {
          Register::RDI, Register::RSI, Register::RDX,
          Register::RCX, Register::R8,  Register::R9};
      const std::vector<x86::Xmm> fp_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      if (num_floating_point_args_ >= fp_arg_regs.size()) {
        throw std::runtime_error("Unsupported. Too many floating point args.");
      }

      if (num_regular_args_ >= normal_arg_regs.size()) {
        throw std::runtime_error("Unsupported. Too many regular args.");
      }

      if (dest_is_reg) {
        // assume register allocator has preassigned the correct slot for
        // FUNC_ARG
        if (type_manager.IsF64Type(type)) {
          assert(fp_registers[dest_reg] ==
                 fp_arg_regs[num_floating_point_args_]);
          num_floating_point_args_++;
        } else {
          assert(normal_registers[dest_reg].GetQ() ==
                 normal_arg_regs[num_regular_args_].GetQ());
          num_regular_args_++;
        }
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;

        if (type_manager.IsF64Type(type)) {
          asm_->movsd(x86::qword_ptr(x86::rbp, offset),
                      fp_arg_regs[num_floating_point_args_]);
          num_floating_point_args_++;
        } else {
          if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
            asm_->mov(x86::byte_ptr(x86::rbp, offset),
                      normal_arg_regs[num_regular_args_].GetB());
          } else if (type_manager.IsI16Type(type)) {
            asm_->mov(x86::word_ptr(x86::rbp, offset),
                      normal_arg_regs[num_regular_args_].GetW());
          } else if (type_manager.IsI32Type(type)) {
            asm_->mov(x86::dword_ptr(x86::rbp, offset),
                      normal_arg_regs[num_regular_args_].GetD());
          } else if (type_manager.IsI64Type(type) ||
                     type_manager.IsPtrType(type)) {
            asm_->mov(x86::qword_ptr(x86::rbp, offset),
                      normal_arg_regs[num_regular_args_].GetQ());
          } else {
            throw std::runtime_error("Invalid argument type.");
          }
          num_regular_args_++;
        }
      }
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
      int32_t dynamic_stack_alloc = 0;
      const std::vector<Register> normal_arg_regs = {
          Register::RDI, Register::RSI, Register::RDX,
          Register::RCX, Register::R8,  Register::R9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5};

      // Save all caller save registers (except for scratch)
      const std::vector<int> caller_saved_normal_registers = {1, 2, 3, 4,
                                                              5, 6, 7};
      const std::vector<int> caller_saved_fp_registers = {0, 1, 2, 3, 4, 5, 6};
      std::unordered_map<int, int> caller_saved_normal_reg_offset;
      std::unordered_map<int, int> caller_saved_fp_reg_offset;

      asm_->sub(x86::rsp, 8 * caller_saved_normal_registers.size() +
                              8 * caller_saved_fp_registers.size());

      for (int i = 0; i < caller_saved_normal_registers.size(); i++) {
        auto reg = caller_saved_normal_registers[i];
        auto offset = 8 * i;
        caller_saved_normal_reg_offset[reg] = offset;
        asm_->mov(x86::qword_ptr(x86::rsp, offset),
                  normal_registers[reg].GetQ());
      }

      for (int i = 0; i < caller_saved_fp_registers.size(); i++) {
        auto reg = caller_saved_fp_registers[i];
        auto offset = 8 * caller_saved_normal_registers.size() + 8 * i;
        caller_saved_fp_reg_offset[reg] = offset;
        asm_->movsd(x86::qword_ptr(x86::rsp, offset), fp_registers[reg]);
      }

      // pass f64 args
      int float_arg_idx = 0;
      while (float_arg_idx < floating_point_call_args_.size() &&
             float_arg_idx < float_arg_regs.size()) {
        auto v = floating_point_call_args_[float_arg_idx];
        bool v_is_reg =
            !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
        int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

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
        } else if (v_is_reg) {
          asm_->movsd(
              float_arg_regs[float_arg_idx],
              x86::qword_ptr(x86::rsp, caller_saved_fp_reg_offset[v_reg]));
        } else {
          asm_->movsd(float_arg_regs[float_arg_idx],
                      x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        }
        float_arg_idx++;
      }
      if (float_arg_idx < floating_point_call_args_.size()) {
        throw std::runtime_error("Too many floating point args for the call.");
      }

      // pass regular args
      int regular_arg_idx = 0;
      while (regular_arg_idx < normal_arg_regs.size() &&
             regular_arg_idx < regular_call_args_.size()) {
        auto [v, type] = regular_call_args_[regular_arg_idx];
        bool v_is_reg =
            !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
        int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

        if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
          if (v.IsConstantGlobal()) {
            int8_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(normal_arg_regs[regular_arg_idx].GetB(), c);
          } else if (v_is_reg) {
            // check to see if its in a caller saved register
            // if so, we need to read it from the stack
            // else, we can just read it from the normal place
            if (caller_saved_normal_reg_offset.find(v_reg) ==
                caller_saved_normal_reg_offset.end()) {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetB(),
                        normal_registers[v_reg].GetB());
            } else {
              asm_->mov(
                  normal_arg_regs[regular_arg_idx].GetQ(),
                  x86::qword_ptr(x86::rsp,
                                 caller_saved_normal_reg_offset.at(v_reg)));
            }
          } else {
            asm_->mov(normal_arg_regs[regular_arg_idx].GetB(),
                      x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          }

          regular_arg_idx++;
          continue;
        }

        if (type_manager.IsI16Type(type)) {
          if (v.IsConstantGlobal()) {
            int16_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(normal_arg_regs[regular_arg_idx].GetW(), c);
          } else if (v_is_reg) {
            // check to see if its in a caller saved register
            // if so, we need to read it from the stack
            // else, we can just read it from the normal place
            if (caller_saved_normal_reg_offset.find(v_reg) ==
                caller_saved_normal_reg_offset.end()) {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetW(),
                        normal_registers[v_reg].GetW());
            } else {
              asm_->mov(
                  normal_arg_regs[regular_arg_idx].GetQ(),
                  x86::qword_ptr(x86::rsp,
                                 caller_saved_normal_reg_offset.at(v_reg)));
            }
          } else {
            asm_->mov(normal_arg_regs[regular_arg_idx].GetW(),
                      x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          }

          regular_arg_idx++;
          continue;
        }

        if (type_manager.IsI32Type(type)) {
          if (v.IsConstantGlobal()) {
            int32_t c =
                Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
            asm_->mov(normal_arg_regs[regular_arg_idx].GetD(), c);
          } else if (v_is_reg) {
            // check to see if its in a caller saved register
            // if so, we need to read it from the stack
            // else, we can just read it from the normal place
            if (caller_saved_normal_reg_offset.find(v_reg) ==
                caller_saved_normal_reg_offset.end()) {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetD(),
                        normal_registers[v_reg].GetD());
            } else {
              asm_->mov(
                  normal_arg_regs[regular_arg_idx].GetQ(),
                  x86::qword_ptr(x86::rsp,
                                 caller_saved_normal_reg_offset.at(v_reg)));
            }
          } else {
            asm_->mov(normal_arg_regs[regular_arg_idx].GetD(),
                      x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          }

          regular_arg_idx++;
          continue;
        }

        if (type_manager.IsI64Type(type)) {
          if (v.IsConstantGlobal()) {
            int64_t c = i64_constants[Type1InstructionReader(
                                          constant_instrs[v.GetIdx()])
                                          .Constant()];
            asm_->mov(normal_arg_regs[regular_arg_idx].GetQ(), c);
          } else if (v_is_reg) {
            // check to see if its in a caller saved register
            // if so, we need to read it from the stack
            // else, we can just read it from the normal place
            if (caller_saved_normal_reg_offset.find(v_reg) ==
                caller_saved_normal_reg_offset.end()) {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetQ(),
                        normal_registers[v_reg].GetQ());
            } else {
              asm_->mov(
                  normal_arg_regs[regular_arg_idx].GetQ(),
                  x86::qword_ptr(x86::rsp,
                                 caller_saved_normal_reg_offset.at(v_reg)));
            }
          } else {
            asm_->mov(normal_arg_regs[regular_arg_idx].GetQ(),
                      x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
          }

          regular_arg_idx++;
          continue;
        }

        if (type_manager.IsPtrType(type)) {
          int32_t ptr_offset = 0;

          if (IsGep(v, instructions)) {
            auto [ptr, o] =
                Gep(v, instructions, constant_instrs, i64_constants);
            v = ptr;
            ptr_offset = o;
          }

          bool v_is_reg =
              !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
          int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

          if (v.IsConstantGlobal()) {
            if (ConstantOpcodeFrom(
                    GenericInstructionReader(constant_instrs[v.GetIdx()])
                        .Opcode()) == ConstantOpcode::NULLPTR) {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetQ(), ptr_offset);
            } else {
              auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
              asm_->lea(normal_arg_regs[regular_arg_idx].GetQ(),
                        x86::ptr(label, ptr_offset));
            }
          } else if (v_is_reg) {
            // check to see if its in a caller saved register
            // if so, we need to read it from the stack
            // else, we can just read it from the normal place
            if (caller_saved_normal_reg_offset.find(v_reg) ==
                caller_saved_normal_reg_offset.end()) {
              asm_->lea(normal_arg_regs[regular_arg_idx].GetQ(),
                        x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
            } else {
              asm_->mov(
                  x86::r10,
                  x86::qword_ptr(x86::rsp,
                                 caller_saved_normal_reg_offset.at(v_reg)));
              asm_->lea(normal_arg_regs[regular_arg_idx].GetQ(),
                        x86::ptr(x86::r10, ptr_offset));
            }
          } else {
            if (ptr_offset != 0) {
              asm_->mov(x86::r10,
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
              asm_->lea(normal_arg_regs[regular_arg_idx].GetQ(),
                        x86::ptr(x86::r10, ptr_offset));
            } else {
              asm_->mov(normal_arg_regs[regular_arg_idx].GetQ(),
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            }
          }
          regular_arg_idx++;
          continue;
        }

        throw std::runtime_error("Invalid argument type.");
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

          bool v_is_reg =
              !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
          int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

          if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
            if (v.IsConstantGlobal()) {
              int8_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                             .Constant();
              asm_->mov(x86::rax, c);
            } else if (v_is_reg) {
              // check to see if its in a caller saved register
              // if so, we need to read it from the stack
              // else, we can just read it from the normal place
              if (caller_saved_normal_reg_offset.find(v_reg) ==
                  caller_saved_normal_reg_offset.end()) {
                asm_->movzx(x86::rax, normal_registers[v_reg].GetB());
              } else {
                asm_->mov(
                    x86::rax,
                    x86::qword_ptr(x86::rsp,
                                   caller_saved_normal_reg_offset.at(v_reg)));
              }
            } else {
              asm_->movzx(x86::rax,
                          x86::byte_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            }

            asm_->push(x86::rax);
            dynamic_stack_alloc += 8;
            continue;
          }

          if (type_manager.IsI16Type(type)) {
            if (v.IsConstantGlobal()) {
              int16_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant();
              asm_->mov(x86::rax, c);
            } else if (v_is_reg) {
              // check to see if its in a caller saved register
              // if so, we need to read it from the stack
              // else, we can just read it from the normal place
              if (caller_saved_normal_reg_offset.find(v_reg) ==
                  caller_saved_normal_reg_offset.end()) {
                asm_->movzx(x86::rax, normal_registers[v_reg].GetW());
              } else {
                asm_->mov(
                    x86::rax,
                    x86::qword_ptr(x86::rsp,
                                   caller_saved_normal_reg_offset.at(v_reg)));
              }
            } else {
              asm_->movzx(x86::rax,
                          x86::word_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            }

            asm_->push(x86::rax);
            dynamic_stack_alloc += 8;
            continue;
          }

          if (type_manager.IsI32Type(type)) {
            if (v.IsConstantGlobal()) {
              int32_t c = Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant();
              asm_->mov(x86::rax, c);
            } else if (v_is_reg) {
              // check to see if its in a caller saved register
              // if so, we need to read it from the stack
              // else, we can just read it from the normal place
              if (caller_saved_normal_reg_offset.find(v_reg) ==
                  caller_saved_normal_reg_offset.end()) {
                asm_->movzx(x86::rax, normal_registers[v_reg].GetD());
              } else {
                asm_->mov(
                    x86::rax,
                    x86::qword_ptr(x86::rsp,
                                   caller_saved_normal_reg_offset.at(v_reg)));
              }
            } else {
              asm_->movzx(x86::rax,
                          x86::dword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            }

            asm_->push(x86::rax);
            dynamic_stack_alloc += 8;
            continue;
          }

          if (type_manager.IsI64Type(type)) {
            if (v.IsConstantGlobal()) {
              int64_t c = i64_constants[Type1InstructionReader(
                                            constant_instrs[v.GetIdx()])
                                            .Constant()];
              asm_->mov(x86::rax, c);
            } else if (v_is_reg) {
              // check to see if its in a caller saved register
              // if so, we need to read it from the stack
              // else, we can just read it from the normal place
              if (caller_saved_normal_reg_offset.find(v_reg) ==
                  caller_saved_normal_reg_offset.end()) {
                asm_->mov(x86::rax, normal_registers[v_reg].GetQ());
              } else {
                asm_->mov(
                    x86::rax,
                    x86::qword_ptr(x86::rsp,
                                   caller_saved_normal_reg_offset.at(v_reg)));
              }
            } else {
              asm_->mov(x86::rax,
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
            }

            asm_->push(x86::rax);
            dynamic_stack_alloc += 8;
            continue;
          }

          if (type_manager.IsPtrType(type)) {
            int32_t ptr_offset = 0;

            if (IsGep(v, instructions)) {
              auto [ptr, o] =
                  Gep(v, instructions, constant_instrs, i64_constants);
              v = ptr;
              ptr_offset = o;
            }

            bool v_is_reg =
                !v.IsConstantGlobal() && register_assign[v.GetIdx()] >= 0;
            int v_reg = v_is_reg ? register_assign[v.GetIdx()] : 0;

            if (v.IsConstantGlobal()) {
              if (ConstantOpcodeFrom(
                      GenericInstructionReader(constant_instrs[v.GetIdx()])
                          .Opcode()) == ConstantOpcode::NULLPTR) {
                asm_->mov(x86::rax, ptr_offset);
              } else {
                auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
                asm_->lea(x86::rax, x86::ptr(label, ptr_offset));
              }
            } else if (v_is_reg) {
              asm_->mov(x86::r10,
                        x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
              // check to see if its in a caller saved register
              // if so, we need to read it from the stack
              // else, we can just read it from the normal place
              if (caller_saved_normal_reg_offset.find(v_reg) ==
                  caller_saved_normal_reg_offset.end()) {
                asm_->lea(x86::rax,
                          x86::ptr(normal_registers[v_reg].GetQ(), ptr_offset));
              } else {
                asm_->mov(
                    x86::r10,
                    x86::qword_ptr(x86::rsp,
                                   caller_saved_normal_reg_offset.at(v_reg)));
                asm_->lea(x86::rax, x86::ptr(x86::r10, ptr_offset));
              }
            } else {
              if (ptr_offset != 0) {
                asm_->mov(x86::r10,
                          x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
                asm_->lea(x86::rax, x86::ptr(x86::r10, ptr_offset));
              } else {
                asm_->mov(x86::rax,
                          x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
              }
            }

            asm_->push(x86::rax);
            dynamic_stack_alloc += 8;
            continue;
          }

          throw std::runtime_error("Invalid argument type.");
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

        asm_->mov(x86::rax, x86::qword_ptr(x86::rbp, Get(offsets, v.GetIdx())));
        asm_->call(x86::rax);

        return_type = type_manager.GetFunctionReturnType(
            static_cast<Type>(reader.TypeID()));
      }

      if (dynamic_stack_alloc != 0) {
        asm_->add(x86::rsp, dynamic_stack_alloc);
      }

      asm_->movsd(x86::xmm7, x86::xmm0);
      for (int i = 0; i < caller_saved_normal_registers.size(); i++) {
        auto reg = caller_saved_normal_registers[i];
        auto offset = caller_saved_normal_reg_offset.at(reg);
        asm_->mov(normal_registers[reg].GetQ(),
                  x86::qword_ptr(x86::rsp, offset));
      }

      for (int i = 0; i < caller_saved_fp_registers.size(); i++) {
        auto reg = caller_saved_fp_registers[i];
        auto offset = caller_saved_fp_reg_offset.at(reg);
        asm_->movsd(fp_registers[reg], x86::qword_ptr(x86::rsp, offset));
      }
      asm_->add(x86::rsp, 8 * caller_saved_normal_registers.size() +
                              8 * caller_saved_fp_registers.size());

      if (!type_manager.IsVoid(return_type)) {
        int32_t offset = INT32_MAX;
        if (!dest_is_reg) {
          offset = stack_allocator.AllocateSlot();
          offsets[instr_idx] = offset;
        }

        if (type_manager.IsF64Type(return_type)) {
          if (dest_is_reg) {
            asm_->movsd(fp_registers[dest_reg], x86::xmm7);
          } else {
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
          }
        } else {
          if (dest_is_reg) {
            asm_->mov(normal_registers[dest_reg].GetQ(), x86::rax);
          } else {
            asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rax);
          }
        }
      }
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