#include "khir/asm/asm_backend.h"

#include <iostream>
#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "khir/asm/live_intervals.h"
#include "khir/asm/register_alloc.h"
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

    if (func.Public()) {
      public_fns_[func.Name()] = internal_func_labels_[func_idx];
    }

    if (func.Name() == "compute") {
      compute_label_ = internal_func_labels_[func_idx];
    }

    auto [live_intervals, order] = ComputeLiveIntervals(func, type_manager);
    // auto register_assign =
    //    AssignRegisters(live_intervals, instructions, type_manager);
    auto register_assign = StackSpillingRegisterAlloc(instructions);

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
    const std::vector<uint64_t>& constant_instrs) {
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

int32_t GetOffset(const std::vector<int32_t>& offsets, int i) {
  auto offset = offsets[i];
  if (offset == INT32_MAX) {
    throw std::runtime_error("Undefined offset.");
  }

  return offset;
}

x86::Xmm ASMBackend::FPRegister(int id) {
  switch (id) {
    case 50:
      return x86::xmm0;
    case 51:
      return x86::xmm1;
    case 52:
      return x86::xmm2;
    case 53:
      return x86::xmm3;
    case 54:
      return x86::xmm4;
    case 55:
      return x86::xmm5;
    case 56:
      return x86::xmm6;
    default:
      throw std::runtime_error("Unreachable.");
  }
}

Register ASMBackend::NormalRegister(int id) {
  /* RBX  = 0
    RCX  = 1
    RDX  = 2
    RSI  = 3
    RDI  = 4
    R8   = 5
    R9   = 6
    R10  = 7
    R11  = 8
    R12  = 9
    R13  = 10
    R14  = 11
    R15  = 12 */
  switch (id) {
    case 0:
      return Register::RBX;
    case 1:
      return Register::RCX;
    case 2:
      return Register::RDX;
    case 3:
      return Register::RSI;
    case 4:
      return Register::RDI;
    case 5:
      return Register::R8;
    case 6:
      return Register::R9;
    case 7:
      return Register::R10;
    case 8:
      return Register::R11;
    case 9:
      return Register::R12;
    case 10:
      return Register::R13;
    case 11:
      return Register::R14;
    case 12:
      return Register::R15;
    default:
      throw std::runtime_error("Unreachable.");
  }
}

asmjit::Label ASMBackend::EmbedF64(double c) {
  auto label = asm_->newLabel();
  asm_->section(data_section_);
  asm_->bind(label);
  asm_->embedDouble(c);
  asm_->section(text_section_);
  return label;
}

asmjit::Label ASMBackend::EmbedI8(int8_t c) {
  auto label = asm_->newLabel();
  asm_->section(data_section_);
  asm_->bind(label);
  asm_->embedInt8(c);
  asm_->section(text_section_);
  return label;
}

asmjit::Label ASMBackend::EmbedI64(int64_t c) {
  auto label = asm_->newLabel();
  asm_->section(data_section_);
  asm_->bind(label);
  asm_->embedInt64(c);
  asm_->section(text_section_);
  return label;
}

bool IsFlagReg(const RegisterAssignment& reg) {
  return reg.IsRegister() && reg.Register() == 100;
}

bool IsF64FlagReg(const RegisterAssignment& reg) {
  return reg.IsRegister() && reg.Register() == 101;
}

template <typename Dest>
void ASMBackend::StoreCmpFlags(Opcode opcode, Dest d) {
  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ: {
      asm_->sete(d);
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE: {
      asm_->setne(d);
      return;
    }

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT: {
      asm_->setl(d);
      return;
    }

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE: {
      asm_->setle(d);
      return;
    }

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT: {
      asm_->setg(d);
      return;
    }

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE: {
      asm_->setge(d);
      return;
    }

    default:
      throw std::runtime_error("Not possible");
  }
}

template <typename T>
void ASMBackend::MoveByteValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->mov(dest, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetB()) {
      asm_->mov(dest, v_reg.GetB());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetB());
    } else {
      asm_->mov(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AddByteValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->add(dest, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, Register::RAX.GetB());
    } else {
      asm_->add(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::SubByteValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->sub(dest, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, Register::RAX.GetB());
    } else {
      asm_->sub(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::MulByteValue(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    auto label = EmbedI8(c8);
    asm_->imul(x86::byte_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->imul(v_reg.GetB());
  } else {
    asm_->imul(x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::GpbLo ASMBackend::GetByteValue(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    int32_t c32 = c8;
    asm_->mov(Register::RAX.GetD(), c32);
    return Register::RAX.GetB();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return NormalRegister(register_assign[v.GetIdx()].Register()).GetB();
  } else {
    asm_->movsx(Register::RAX.GetD(),
                x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return Register::RAX.GetB();
  }
}

void ASMBackend::CmpByteValue(
    x86::GpbLo src, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->cmp(src, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetB());
  } else {
    asm_->cmp(src, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::ZextByteValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    uint8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    uint64_t c64 = c8;
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->movzx(dest, v_reg.GetB());
  } else {
    asm_->movzx(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetBytePtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  int32_t offset = 0;
  if (IsGep(v, instrs)) {
    auto [ptr, o] = Gep(v, instrs, constant_instrs);
    v = ptr;
    offset = o;
  }

  if (v.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
    return x86::byte_ptr(label, offset);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    return x86::byte_ptr(v_reg.GetQ(), offset);
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::byte_ptr(Register::RAX.GetQ(), offset);
  }
}

template <typename T>
void ASMBackend::MoveWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->mov(dest, c16);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetW()) {
      asm_->mov(dest, v_reg.GetW());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetW());
    } else {
      asm_->movsx(dest,
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AddWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->add(dest, c16);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetW());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, Register::RAX.GetW());
    } else {
      asm_->add(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::SubWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->sub(dest, c16);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetW());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, Register::RAX.GetW());
    } else {
      asm_->sub(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::MulWordValue(
    x86::Gpw dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->imul(dest, dest, c16);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->imul(dest, v_reg.GetW());
  } else {
    asm_->imul(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Gpw ASMBackend::GetWordValue(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    int32_t c32 = c16;
    asm_->mov(Register::RAX.GetD(), c32);
    return Register::RAX.GetW();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return NormalRegister(register_assign[v.GetIdx()].Register()).GetW();
  } else {
    asm_->movsx(Register::RAX.GetD(),
                x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return Register::RAX.GetW();
  }
}

void ASMBackend::CmpWordValue(
    x86::Gpw src, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->cmp(src, c16);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetW());
  } else {
    asm_->cmp(src, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::ZextWordValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    uint16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    uint64_t c64 = c16;
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->movzx(dest, v_reg.GetW());
  } else {
    asm_->movzx(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  int32_t offset = 0;
  if (IsGep(v, instrs)) {
    auto [ptr, o] = Gep(v, instrs, constant_instrs);
    v = ptr;
    offset = o;
  }

  if (v.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
    return x86::word_ptr(label, offset);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    return x86::word_ptr(v_reg.GetQ(), offset);
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::word_ptr(Register::RAX.GetQ(), offset);
  }
}

template <typename T>
void ASMBackend::MoveDWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->mov(dest, c32);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetD()) {
      asm_->mov(dest, v_reg.GetD());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetD());
    } else {
      asm_->movsx(dest,
                  x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AddDWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->add(dest, c32);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetD());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, Register::RAX.GetD());
    } else {
      asm_->add(dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::SubDWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->sub(dest, c32);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetD());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(Register::RAX.GetD(),
                  x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, Register::RAX.GetD());
    } else {
      asm_->sub(dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::MulDWordValue(
    x86::Gpd dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->imul(dest, dest, c32);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->imul(dest, v_reg.GetD());
  } else {
    asm_->imul(dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Gpd ASMBackend::GetDWordValue(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->mov(Register::RAX.GetD(), c32);
    return Register::RAX.GetD();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return NormalRegister(register_assign[v.GetIdx()].Register()).GetD();
  } else {
    asm_->mov(Register::RAX.GetD(),
              x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return Register::RAX.GetD();
  }
}

void ASMBackend::CmpDWordValue(
    x86::Gpd src, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->cmp(src, c32);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetD());
  } else {
    asm_->cmp(src, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::ZextDWordValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    uint32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    uint64_t c64 = c32;
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->movzx(dest, v_reg.GetD());
  } else {
    asm_->movzx(dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetDWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  int32_t offset = 0;
  if (IsGep(v, instrs)) {
    auto [ptr, o] = Gep(v, instrs, constant_instrs);
    v = ptr;
    offset = o;
  }

  if (v.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
    return x86::dword_ptr(label, offset);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    return x86::dword_ptr(v_reg.GetQ(), offset);
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::dword_ptr(Register::RAX.GetQ(), offset);
  }
}

template <typename T>
void ASMBackend::MovePtrValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
    asm_->lea(Register::RAX.GetQ(), x86::ptr(label));
    asm_->mov(dest, Register::RAX.GetQ());
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetQ()) {
      asm_->mov(dest, v_reg.GetQ());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(Register::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetQ());
    } else {
      asm_->mov(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::MoveQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetQ()) {
      asm_->mov(dest, v_reg.GetQ());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(Register::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetQ());
    } else {
      asm_->mov(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AddQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if (c64 >= INT32_MIN && c64 <= INT32_MAX) {
      int32_t c32 = c64;
      asm_->add(dest, c32);
    } else {
      if constexpr (std::is_same_v<T, x86::Mem>) {
        asm_->mov(x86::rax, c64);
        asm_->add(dest, x86::rax);
      } else {
        asm_->add(dest, x86::qword_ptr(EmbedI64(c64)));
      }
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(Register::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, Register::RAX.GetQ());
    } else {
      asm_->add(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::SubQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if (c64 >= INT32_MIN && c64 <= INT32_MAX) {
      int32_t c32 = c64;
      asm_->sub(dest, c32);
    } else {
      if constexpr (std::is_same_v<T, x86::Mem>) {
        asm_->mov(x86::rax, c64);
        asm_->sub(dest, x86::rax);
      } else {
        asm_->sub(dest, x86::qword_ptr(EmbedI64(c64)));
      }
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(Register::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, Register::RAX.GetQ());
    } else {
      asm_->sub(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::MulQWordValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if (c64 >= INT32_MIN && c64 <= INT32_MAX) {
      int32_t c32 = c64;
      asm_->imul(dest, dest, c32);
    } else {
      asm_->imul(dest, x86::qword_ptr(EmbedI64(c64)));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->imul(dest, v_reg.GetQ());
  } else {
    asm_->imul(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Gpq ASMBackend::GetQWordValue(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    asm_->mov(Register::RAX.GetQ(), c64);
    return Register::RAX.GetQ();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return NormalRegister(register_assign[v.GetIdx()].Register()).GetQ();
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return Register::RAX.GetQ();
  }
}

void ASMBackend::CmpQWordValue(
    x86::Gpq src, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    if (c64 >= INT32_MIN && c64 <= INT32_MAX) {
      int32_t c32 = c64;
      asm_->cmp(src, c32);
    } else {
      asm_->cmp(src, x86::qword_ptr(EmbedI64(c64)));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetQ());
  } else {
    asm_->cmp(src, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetQWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  int32_t offset = 0;
  if (IsGep(v, instrs)) {
    auto [ptr, o] = Gep(v, instrs, constant_instrs);
    v = ptr;
    offset = o;
  }

  if (v.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
    return x86::qword_ptr(label, offset);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
    return x86::qword_ptr(v_reg.GetQ(), offset);
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::qword_ptr(Register::RAX.GetQ(), offset);
  }
}

void ASMBackend::MaterializeGep(
    x86::Mem dest, khir::Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  assert(IsGep(v, instrs));
  auto [ptr, offset] = Gep(v, instrs, constant_instrs);

  if (ptr.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[ptr.GetIdx()]);
    asm_->lea(Register::RAX.GetQ(), x86::ptr(label, offset));
    asm_->mov(dest, Register::RAX.GetQ());
  } else if (register_assign[ptr.GetIdx()].IsRegister()) {
    auto ptr_reg = NormalRegister(register_assign[ptr.GetIdx()].Register());
    asm_->mov(dest, ptr_reg.GetQ());
    asm_->add(dest, offset);
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, ptr.GetIdx())));
    asm_->mov(dest, Register::RAX.GetQ());
    asm_->add(dest, offset);
  }
}

void ASMBackend::MaterializeGep(
    x86::Gpq dest, khir::Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  assert(IsGep(v, instrs));
  auto [ptr, offset] = Gep(v, instrs, constant_instrs);

  if (ptr.IsConstantGlobal()) {
    auto label = GetConstantGlobal(constant_instrs[ptr.GetIdx()]);
    asm_->lea(dest, x86::ptr(label, offset));
  } else if (register_assign[ptr.GetIdx()].IsRegister()) {
    auto ptr_reg = NormalRegister(register_assign[ptr.GetIdx()].Register());
    asm_->lea(dest, x86::ptr(ptr_reg.GetQ(), offset));
  } else {
    asm_->mov(Register::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, ptr.GetIdx())));
    asm_->lea(dest, x86::ptr(Register::RAX.GetQ(), offset));
  }
}

template <typename T>
void ASMBackend::MoveF64Value(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsd(x86::xmm7, x86::qword_ptr(label));
      asm_->movsd(dest, x86::xmm7);
    } else {
      asm_->movsd(dest, x86::qword_ptr(label));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = FPRegister(register_assign[v.GetIdx()].Register());
    asm_->movsd(dest, reg);
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsd(x86::xmm7,
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->movsd(dest, x86::xmm7);
    } else {
      asm_->movsd(dest,
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::AddF64Value(
    x86::Xmm dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->addsd(dest, x86::qword_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = FPRegister(register_assign[v.GetIdx()].Register());
    asm_->addsd(dest, reg);
  } else {
    asm_->addsd(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::SubF64Value(
    x86::Xmm dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->subsd(dest, x86::qword_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = FPRegister(register_assign[v.GetIdx()].Register());
    asm_->subsd(dest, reg);
  } else {
    asm_->subsd(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::MulF64Value(
    x86::Xmm dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->mulsd(dest, x86::qword_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = FPRegister(register_assign[v.GetIdx()].Register());
    asm_->mulsd(dest, reg);
  } else {
    asm_->mulsd(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::DivF64Value(
    x86::Xmm dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->divsd(dest, x86::qword_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = FPRegister(register_assign[v.GetIdx()].Register());
    asm_->divsd(dest, reg);
  } else {
    asm_->divsd(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

template <typename Dest>
void ASMBackend::StoreF64CmpFlags(Opcode opcode, Dest dest) {
  switch (opcode) {
    case Opcode::F64_CMP_EQ: {
      asm_->setnp(dest);
      asm_->sete(x86::al);
      asm_->and_(dest, x86::al);
      return;
    }

    case Opcode::F64_CMP_NE: {
      asm_->setnp(dest);
      asm_->setne(x86::al);
      asm_->or_(dest, x86::al);
      return;
    }

    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_GT: {
      asm_->seta(dest);
      return;
    }

    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GE: {
      asm_->setae(dest);
      return;
    }

    default:
      throw std::runtime_error("Impossible");
  }
}

x86::Xmm ASMBackend::GetF64Value(
    Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->movsd(x86::xmm7, x86::qword_ptr(label));
    return x86::xmm7;
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return FPRegister(register_assign[v.GetIdx()].Register());
  } else {
    asm_->movsd(x86::xmm7,
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::xmm7;
  }
}

void ASMBackend::CmpF64Value(
    asmjit::x86::Xmm src, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    auto label = EmbedF64(cf64);
    asm_->ucomisd(src, x86::qword_ptr(label));
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    asm_->ucomisd(src, FPRegister(register_assign[v.GetIdx()].Register()));
  } else {
    asm_->ucomisd(src,
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

template <typename T>
void ASMBackend::F64ConvI64Value(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<double>& f64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    double cf64 =
        f64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];
    int64_t c64 = cf64;

    if constexpr (std::is_same_v<x86::Mem, T>) {
      if (c64 >= INT32_MIN && c64 <= INT32_MAX) {
        int32_t c32 = c64;
        asm_->mov(dest, c32);
      } else {
        asm_->mov(Register::RAX.GetQ(), c64);
        asm_->mov(dest, Register::RAX.GetQ());
      }
    } else {
      asm_->mov(dest, c64);
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    if constexpr (std::is_same_v<x86::Mem, T>) {
      asm_->cvttsd2si(Register::RAX.GetQ(),
                      FPRegister(register_assign[v.GetIdx()].Register()));
      asm_->mov(dest, Register::RAX.GetQ());
    } else {
      asm_->cvttsd2si(dest, FPRegister(register_assign[v.GetIdx()].Register()));
    }
  } else {
    if constexpr (std::is_same_v<x86::Mem, T>) {
      asm_->cvttsd2si(Register::RAX.GetQ(),
                      x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, Register::RAX.GetQ());
    } else {
      asm_->cvttsd2si(dest,
                      x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::CondBrFlag(
    Value v, const asmjit::Label& tr, const asmjit::Label& fl,
    const std::vector<uint64_t>& instructions,
    const std::vector<RegisterAssignment>& register_assign) {
  assert(!v.IsConstantGlobal());
  GenericInstructionReader reader(instructions[v.GetIdx()]);
  auto opcode = OpcodeFrom(reader.Opcode());
  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ: {
      asm_->je(tr);
      asm_->jmp(fl);
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE: {
      asm_->jne(tr);
      asm_->jmp(fl);
      return;
    }

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT: {
      asm_->jl(tr);
      asm_->jmp(fl);
      return;
    }

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE: {
      asm_->jle(tr);
      asm_->jmp(fl);
      return;
    }

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT: {
      asm_->jg(tr);
      asm_->jmp(fl);
      return;
    }

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE: {
      asm_->jge(tr);
      asm_->jmp(fl);
      return;
    }

    default:
      throw std::runtime_error("Not possible");
  }
}

void ASMBackend::CondBrF64Flag(
    Value v, const asmjit::Label& tr, const asmjit::Label& fl,
    const std::vector<uint64_t>& instructions,
    const std::vector<RegisterAssignment>& register_assign) {
  assert(!v.IsConstantGlobal());
  GenericInstructionReader reader(instructions[v.GetIdx()]);
  auto opcode = OpcodeFrom(reader.Opcode());
  switch (opcode) {
    case Opcode::F64_CMP_EQ: {
      asm_->jne(fl);
      asm_->jp(fl);
      asm_->jmp(tr);
      return;
    }

    case Opcode::F64_CMP_NE: {
      asm_->jne(tr);
      asm_->jnp(fl);
      asm_->jmp(tr);
      return;
    }

    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_GT: {
      asm_->jbe(fl);
      asm_->jmp(tr);
      return;
    }

    case Opcode::F64_CMP_GE:
    case Opcode::F64_CMP_LE: {
      asm_->jae(tr);
      asm_->jmp(fl);
      return;
    }

    default:
      throw std::runtime_error("Not possible");
  }
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

  const auto& dest_assign = register_assign[instr_idx];

  switch (opcode) {
    // I1
    // ======================================================================
    case Opcode::I1_ZEXT_I8: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetB();
        MoveByteValue(dest, v, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v, offsets, constant_instrs, register_assign);
      }

      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetB();
        MoveByteValue(dest, v, offsets, constant_instrs, register_assign);
        asm_->xor_(dest, 1);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v, offsets, constant_instrs, register_assign);
        asm_->xor_(dest, 1);
      }

      return;
    }

    // I8 ======================================================================
    case Opcode::I8_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetB();
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        AddByteValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        AddByteValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I8_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetB();
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        SubByteValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        SubByteValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I8_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      MoveByteValue(Register::RAX.GetB(), v0, offsets, constant_instrs,
                    register_assign);
      MulByteValue(v1, offsets, constant_instrs, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->movsx(NormalRegister(dest_assign.Register()).GetD(),
                    Register::RAX.GetB());
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), Register::RAX.GetB());
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

      auto v0_reg = GetByteValue(v0, offsets, constant_instrs, register_assign);
      CmpByteValue(v0_reg, v1, offsets, constant_instrs, register_assign);

      if (IsFlagReg(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? NormalRegister(dest_assign.Register())
                      : Register::RAX;
      ZextByteValue(dest.GetQ(), v, offsets, constant_instrs, register_assign);

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest.GetQ());
      }
      return;
    }

    case Opcode::I8_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister() ? FPRegister(dest_assign.Register())
                                           : x86::xmm7;

      if (v.IsConstantGlobal()) {
        int8_t c8 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c8;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
        asm_->movsx(x86::eax, v_reg.GetB());
        asm_->cvtsi2sd(dest, x86::eax);
      } else {
        asm_->movsx(x86::eax,
                    x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        asm_->cvtsi2sd(dest, x86::eax);
      }

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
      return;
    }

    case Opcode::I8_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetBytePtrValue(v, offsets, instructions, constant_instrs,
                                 register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetB(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movzx(Register::RAX.GetQ(), loc);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), Register::RAX.GetB());
      }
      return;
    }

    case Opcode::I8_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetBytePtrValue(v0, offsets, instructions, constant_instrs,
                                 register_assign);

      if (v1.IsConstantGlobal()) {
        int8_t c8 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c8);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, NormalRegister(v1_reg).GetB());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = NormalRegister(dest_assign.Register());
        asm_->movzx(dest.GetD(),
                    x86::byte_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->mov(loc, dest.GetB());
      }
      return;
    }

    // I16 =====================================================================
    case Opcode::I16_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetW();
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        AddWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::word_ptr(x86::rbp, offset);
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        AddWordValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I16_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetW();
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        SubWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::word_ptr(x86::rbp, offset);
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        SubWordValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I16_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetW();
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = Register::RAX.GetW();
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulWordValue(dest, v1, offsets, constant_instrs, register_assign);
        asm_->mov(x86::word_ptr(x86::rbp, offset), dest);
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

      auto v0_reg = GetWordValue(v0, offsets, constant_instrs, register_assign);
      CmpWordValue(v0_reg, v1, offsets, constant_instrs, register_assign);

      if (IsFlagReg(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I16_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? NormalRegister(dest_assign.Register())
                      : Register::RAX;
      ZextWordValue(dest.GetQ(), v, offsets, constant_instrs, register_assign);

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest.GetQ());
      }
      return;
    }

    case Opcode::I16_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister() ? FPRegister(dest_assign.Register())
                                           : x86::xmm7;

      if (v.IsConstantGlobal()) {
        int16_t c16 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c16;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
        asm_->movsx(x86::eax, v_reg.GetW());
        asm_->cvtsi2sd(dest, x86::eax);
      } else {
        asm_->movsx(x86::eax,
                    x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        asm_->cvtsi2sd(dest, x86::eax);
      }

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
      return;
    }

    case Opcode::I16_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetWordPtrValue(v, offsets, instructions, constant_instrs,
                                 register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetW(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movzx(Register::RAX.GetQ(), loc);
        asm_->mov(x86::word_ptr(x86::rbp, offset), Register::RAX.GetW());
      }
      return;
    }

    case Opcode::I16_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetWordPtrValue(v0, offsets, instructions, constant_instrs,
                                 register_assign);

      if (v1.IsConstantGlobal()) {
        int16_t c16 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c16);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, NormalRegister(v1_reg).GetW());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = NormalRegister(dest_assign.Register());
        asm_->movzx(dest.GetD(),
                    x86::word_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->mov(loc, dest.GetW());
      }
      return;
    }

    // I32 =====================================================================
    case Opcode::I32_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetD();
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        AddDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::dword_ptr(x86::rbp, offset);
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        AddDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I32_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetD();
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        SubDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::dword_ptr(x86::rbp, offset);
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        SubDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I32_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetD();
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = Register::RAX.GetD();
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulDWordValue(dest, v1, offsets, constant_instrs, register_assign);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), dest);
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

      auto v0_reg =
          GetDWordValue(v0, offsets, constant_instrs, register_assign);
      CmpDWordValue(v0_reg, v1, offsets, constant_instrs, register_assign);

      if (IsFlagReg(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? NormalRegister(dest_assign.Register())
                      : Register::RAX;
      ZextDWordValue(dest.GetQ(), v, offsets, constant_instrs, register_assign);

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest.GetQ());
      }
      return;
    }

    case Opcode::I32_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister() ? FPRegister(dest_assign.Register())
                                           : x86::xmm7;

      if (v.IsConstantGlobal()) {
        int32_t c32 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c32;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
        asm_->cvtsi2sd(dest, v_reg.GetD());
      } else {
        asm_->cvtsi2sd(
            dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      }

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
      return;
    }

    case Opcode::I32_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetDWordPtrValue(v, offsets, instructions, constant_instrs,
                                  register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetD(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(Register::RAX.GetD(), loc);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), Register::RAX.GetD());
      }
      return;
    }

    case Opcode::I32_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetDWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  register_assign);

      if (v1.IsConstantGlobal()) {
        int32_t c32 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c32);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, NormalRegister(v1_reg).GetD());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = NormalRegister(dest_assign.Register());
        asm_->mov(dest.GetD(),
                  x86::dword_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->mov(loc, dest.GetD());
      }
      return;
    }

    // I64 =====================================================================
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        AddQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        AddQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      }
      return;
    }

    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        SubQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        SubQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      }
      return;
    }

    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = NormalRegister(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        MulQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = Register::RAX.GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        MulQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest);
      }
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

      auto v0_reg = GetQWordValue(v0, offsets, constant_instrs, i64_constants,
                                  register_assign);
      CmpQWordValue(v0_reg, v1, offsets, constant_instrs, i64_constants,
                    register_assign);

      if (IsFlagReg(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister() ? FPRegister(dest_assign.Register())
                                           : x86::xmm7;

      if (v.IsConstantGlobal()) {
        int64_t ci64 =
            i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant()];
        double cf64 = ci64;
        auto label = EmbedF64(cf64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
        asm_->cvtsi2sd(dest, v_reg.GetQ());
      } else {
        asm_->cvtsi2sd(
            dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      }

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), dest);
      }
      return;
    }

    case Opcode::I64_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  register_assign);

      if (v1.IsConstantGlobal()) {
        assert(dest_assign.IsRegister());
        int64_t ci64 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        auto dest = NormalRegister(dest_assign.Register());

        if (ci64 >= INT32_MIN && ci64 <= INT32_MAX) {
          int32_t ci32 = ci64;
          asm_->mov(loc, ci32);
        } else {
          asm_->mov(dest.GetQ(), ci64);
          asm_->mov(loc, dest.GetQ());
        }
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, NormalRegister(v1_reg).GetQ());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = NormalRegister(dest_assign.Register());
        asm_->mov(dest.GetQ(),
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->mov(loc, dest.GetQ());
      }
      return;
    }

    case Opcode::I64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetQWordPtrValue(v, offsets, instructions, constant_instrs,
                                  register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetQ(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(Register::RAX.GetQ(), loc);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), Register::RAX.GetQ());
      }
      return;
    }

    // PTR =====================================================================
    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);

      auto ptr_type = static_cast<khir::Type>(reader.TypeID());
      auto type = type_manager.GetPointerElementType(ptr_type);
      auto size = type_manager.GetTypeSize(type);
      size += (16 - (size % 16)) % 16;
      assert(size % 16 == 0);

      asm_->sub(x86::rsp, size);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetQ(), x86::rsp);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rsp);
      }
      return;
    }

    case Opcode::GEP:
    case Opcode::GEP_OFFSET: {
      // do nothing since we lazily compute GEPs
      return;
    }

    case Opcode::PTR_MATERIALIZE: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());

      if (dest_assign.IsRegister()) {
        auto dest_reg = NormalRegister(dest_assign.Register()).GetQ();
        MaterializeGep(dest_reg, v, offsets, instructions, constant_instrs,
                       register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MaterializeGep(x86::qword_ptr(x86::rbp, offset), v, offsets,
                       instructions, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());

      if (dest_assign.IsRegister()) {
        auto dest_reg = NormalRegister(dest_assign.Register()).GetQ();

        if (v.IsConstantGlobal()) {
          auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
          asm_->lea(dest_reg, x86::qword_ptr(label));
        } else if (register_assign[v.GetIdx()].IsRegister()) {
          auto v_reg =
              NormalRegister(register_assign[v.GetIdx()].Register()).GetQ();
          if (dest_reg != v_reg) {
            asm_->mov(dest_reg, v_reg);
          }
        } else {
          asm_->mov(dest_reg,
                    x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        }
        return;
      }

      auto offset = stack_allocator.AllocateSlot();
      offsets[instr_idx] = offset;
      if (v.IsConstantGlobal()) {
        auto label = GetConstantGlobal(constant_instrs[v.GetIdx()]);
        asm_->lea(Register::RAX.GetQ(), x86::ptr(label));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), Register::RAX.GetQ());
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = NormalRegister(register_assign[v.GetIdx()].Register());
        asm_->mov(x86::qword_ptr(x86::rbp, offset), v_reg.GetQ());
      } else {
        asm_->mov(Register::RAX.GetQ(),
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), Register::RAX.GetQ());
      }
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());

      auto loc = GetQWordPtrValue(v, offsets, instructions, constant_instrs,
                                  register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(NormalRegister(dest_assign.Register()).GetQ(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(Register::RAX.GetQ(), loc);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), Register::RAX.GetQ());
      }
      return;
    }

    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  register_assign);

      if (v1.IsConstantGlobal()) {
        assert(dest_assign.IsRegister());
        auto label = GetConstantGlobal(constant_instrs[v0.GetIdx()]);
        auto dest = NormalRegister(dest_assign.Register());
        asm_->lea(dest.GetQ(), x86::ptr(label));
        asm_->mov(loc, dest.GetQ());
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, NormalRegister(v1_reg).GetQ());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = NormalRegister(dest_assign.Register());
        asm_->mov(dest.GetQ(),
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->mov(loc, dest.GetQ());
      }
      return;
    }

    // F64 =====================================================================
    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = FPRegister(dest_assign.Register());
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        AddF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        AddF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = FPRegister(dest_assign.Register());
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        SubF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        SubF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = FPRegister(dest_assign.Register());
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        MulF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        MulF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
      }
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = FPRegister(dest_assign.Register());
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        DivF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        DivF64Value(x86::xmm7, v0, offsets, constant_instrs, f64_constants,
                    register_assign);
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

      auto reg = GetF64Value(v0, offsets, constant_instrs, f64_constants,
                             register_assign);
      CmpF64Value(reg, v1, offsets, constant_instrs, f64_constants,
                  register_assign);

      if (IsF64FlagReg(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetB();
        StoreF64CmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreF64CmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      if (dest_assign.IsRegister()) {
        auto loc = NormalRegister(dest_assign.Register()).GetQ();
        F64ConvI64Value(loc, v, offsets, constant_instrs, f64_constants,
                        register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::qword_ptr(x86::rbp, offset);
        F64ConvI64Value(loc, v, offsets, constant_instrs, f64_constants,
                        register_assign);
      }
      return;
    }

    case Opcode::F64_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  register_assign);

      if (v1.IsConstantGlobal()) {
        double cf64 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->movsd(x86::xmm7, x86::qword_ptr(EmbedF64(cf64)));
        asm_->movsd(loc, x86::xmm7);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = FPRegister(register_assign[v1.GetIdx()].Register());
        asm_->movsd(loc, v1_reg);
      } else {
        asm_->movsd(x86::xmm7,
                    x86::qword_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->movsd(loc, x86::xmm7);
      }
      return;
    }

    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  register_assign);

      if (dest_assign.IsRegister()) {
        asm_->movsd(FPRegister(dest_assign.Register()), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::xmm7, loc);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm7);
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

      if (v.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        if (c != 0) {
          asm_->jmp(basic_blocks[reader.Marg0()]);
        } else {
          asm_->jmp(basic_blocks[reader.Marg1()]);
        }
        return;
      }

      if (IsFlagReg(register_assign[v.GetIdx()])) {
        CondBrFlag(v, basic_blocks[reader.Marg0()],
                   basic_blocks[reader.Marg1()], instructions, register_assign);
      } else if (IsF64FlagReg(register_assign[v.GetIdx()])) {
        CondBrF64Flag(v, basic_blocks[reader.Marg0()],
                      basic_blocks[reader.Marg1()], instructions,
                      register_assign);
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        asm_->cmp(NormalRegister(register_assign[v.GetIdx()].Register()).GetB(),
                  1);
        asm_->je(basic_blocks[reader.Marg0()]);
        asm_->jmp(basic_blocks[reader.Marg1()]);
      } else {
        asm_->cmp(x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())), 1);
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

      if (!dest_assign.IsRegister() && offsets[phi.GetIdx()] == INT32_MAX) {
        offsets[phi.GetIdx()] = stack_allocator.AllocateSlot();
      }

      if (type_manager.IsF64Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = FPRegister(dest_assign.Register());
          MoveF64Value(dst, v, offsets, constant_instrs, f64_constants,
                       register_assign);
        } else {
          auto dst = x86::qword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveF64Value(dst, v, offsets, constant_instrs, f64_constants,
                       register_assign);
        }
        return;
      }

      if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register()).GetB();
          MoveByteValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::byte_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveByteValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI16Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register()).GetW();
          MoveWordValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::word_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveWordValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI32Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register()).GetD();
          MoveDWordValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::dword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveDWordValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI64Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register()).GetQ();
          MoveQWordValue(dst, v, offsets, constant_instrs, i64_constants,
                         register_assign);
        } else {
          auto dst = x86::qword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveQWordValue(dst, v, offsets, constant_instrs, i64_constants,
                         register_assign);
        }
        return;
      }

      if (type_manager.IsPtrType(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register()).GetQ();
          MovePtrValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::qword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MovePtrValue(dst, v, offsets, constant_instrs, register_assign);
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

      if (type_manager.IsF64Type(type)) {
        MoveF64Value(x86::xmm0, v, offsets, constant_instrs, f64_constants,
                     register_assign);
        return;
      }

      if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        MoveByteValue(Register::RAX.GetB(), v, offsets, constant_instrs,
                      register_assign);
        return;
      }

      if (type_manager.IsI16Type(type)) {
        MoveWordValue(Register::RAX.GetW(), v, offsets, constant_instrs,
                      register_assign);
        return;
      }

      if (type_manager.IsI32Type(type)) {
        MoveDWordValue(Register::RAX.GetD(), v, offsets, constant_instrs,
                       register_assign);
        return;
      }

      if (type_manager.IsI64Type(type)) {
        MoveQWordValue(Register::RAX.GetQ(), v, offsets, constant_instrs,
                       i64_constants, register_assign);
        return;
      }

      if (type_manager.IsPtrType(type)) {
        MovePtrValue(Register::RAX.GetQ(), v, offsets, constant_instrs,
                     register_assign);
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

      if (type_manager.IsF64Type(type)) {
        auto src = fp_arg_regs[num_floating_point_args_++];

        if (dest_assign.IsRegister()) {
          auto dst = FPRegister(dest_assign.Register());
          if (dst != src) {
            asm_->movsd(dst, src);
          }
          return;
        }

        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), src);
        return;
      }

      auto src = normal_arg_regs[num_regular_args_++];
      if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register());
          if (dst.GetB() != src.GetB()) {
            asm_->mov(dst.GetB(), src.GetB());
          }
          return;
        }

        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), src.GetB());
        return;
      }

      if (type_manager.IsI16Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register());
          if (dst.GetW() != src.GetW()) {
            asm_->mov(dst.GetW(), src.GetW());
          }
          return;
        }

        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::word_ptr(x86::rbp, offset), src.GetW());
        return;
      }

      if (type_manager.IsI32Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register());
          if (dst.GetD() != src.GetD()) {
            asm_->mov(dst.GetD(), src.GetD());
          }
          return;
        }

        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), src.GetD());
        return;
      }

      if (type_manager.IsI64Type(type) || type_manager.IsPtrType(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = NormalRegister(dest_assign.Register());
          if (dst.GetQ() != src.GetQ()) {
            asm_->mov(dst.GetQ(), src.GetQ());
          }
          return;
        }

        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), src.GetQ());
        return;
      }

      throw std::runtime_error("Invalid argument type.");
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

      // pass f64 args
      int float_arg_idx = 0;
      while (float_arg_idx < floating_point_call_args_.size() &&
             float_arg_idx < float_arg_regs.size()) {
        auto v = floating_point_call_args_[float_arg_idx];
        auto reg = float_arg_regs[float_arg_idx++];
        MoveF64Value(reg, v, offsets, constant_instrs, f64_constants,
                     register_assign);
      }
      if (float_arg_idx < floating_point_call_args_.size()) {
        throw std::runtime_error("Too many floating point args for the call.");
      }

      // pass regular args
      int regular_arg_idx = 0;
      while (regular_arg_idx < normal_arg_regs.size() &&
             regular_arg_idx < regular_call_args_.size()) {
        auto [v, type] = regular_call_args_[regular_arg_idx];
        auto reg = normal_arg_regs[regular_arg_idx++];

        if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
          MoveByteValue(reg.GetB(), v, offsets, constant_instrs,
                        register_assign);
        } else if (type_manager.IsI16Type(type)) {
          MoveWordValue(reg.GetW(), v, offsets, constant_instrs,
                        register_assign);
        } else if (type_manager.IsI32Type(type)) {
          MoveDWordValue(reg.GetD(), v, offsets, constant_instrs,
                         register_assign);
        } else if (type_manager.IsI64Type(type)) {
          MoveQWordValue(reg.GetQ(), v, offsets, constant_instrs, i64_constants,
                         register_assign);
        } else if (type_manager.IsPtrType(type)) {
          MovePtrValue(reg.GetQ(), v, offsets, constant_instrs,
                       register_assign);
        } else {
          throw std::runtime_error("Invalid argument type.");
        }
      }

      // if remaining args exist, pass them right to left
      int num_remaining_args = regular_call_args_.size() - regular_arg_idx;
      if (num_remaining_args > 0) {
        auto stack_alloc_offset = asm_->offset();
        asm_->long_().sub(x86::rsp, 0);

        for (int i = regular_arg_idx; i < regular_call_args_.size(); i++) {
          auto [v, type] = regular_call_args_[i];
          auto offset = dynamic_stack_alloc;
          dynamic_stack_alloc += 8;

          if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
            auto loc = x86::byte_ptr(x86::rsp, offset);
            MoveByteValue(loc, v, offsets, constant_instrs, register_assign);
          } else if (type_manager.IsI16Type(type)) {
            auto loc = x86::word_ptr(x86::rsp, offset);
            MoveWordValue(loc, v, offsets, constant_instrs, register_assign);
          } else if (type_manager.IsI32Type(type)) {
            auto loc = x86::dword_ptr(x86::rsp, offset);
            MoveDWordValue(loc, v, offsets, constant_instrs, register_assign);
          } else if (type_manager.IsI64Type(type)) {
            auto loc = x86::qword_ptr(x86::rsp, offset);
            MoveQWordValue(loc, v, offsets, constant_instrs, i64_constants,
                           register_assign);
          } else if (type_manager.IsPtrType(type)) {
            auto loc = x86::qword_ptr(x86::rsp, offset);
            MovePtrValue(loc, v, offsets, constant_instrs, register_assign);
          } else {
            throw std::runtime_error("Invalid argument type.");
          }
        }

        // add align space to remain 16 byte aligned
        if (num_remaining_args % 2 == 1) {
          dynamic_stack_alloc += 8;
        }

        auto current_offset = asm_->offset();
        asm_->setOffset(stack_alloc_offset);
        asm_->long_().sub(x86::rsp, dynamic_stack_alloc);
        asm_->setOffset(current_offset);
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

        asm_->call(x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        return_type = type_manager.GetFunctionReturnType(
            static_cast<Type>(reader.TypeID()));
      }

      if (dynamic_stack_alloc != 0) {
        asm_->add(x86::rsp, dynamic_stack_alloc);
      }

      if (!type_manager.IsVoid(return_type)) {
        if (type_manager.IsF64Type(return_type)) {
          if (dest_assign.IsRegister()) {
            auto reg = FPRegister(dest_assign.Register());
            if (reg != x86::xmm0) {
              asm_->movsd(reg, x86::xmm0);
            }
          } else {
            auto offset = stack_allocator.AllocateSlot();
            offsets[instr_idx] = offset;
            asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm0);
          }
          return;
        }

        if (dest_assign.IsRegister()) {
          auto reg = NormalRegister(dest_assign.Register());
          if (reg.GetQ() != Register::RAX.GetQ()) {
            asm_->mov(reg.GetQ(), Register::RAX.GetQ());
          }
        } else {
          auto offset = stack_allocator.AllocateSlot();
          offsets[instr_idx] = offset;
          asm_->mov(x86::qword_ptr(x86::rbp, offset), Register::RAX.GetQ());
        }
      }
      return;
    }
  }
}

void ASMBackend::Execute() {
  auto offset = code_.labelOffsetFromBase(compute_label_);
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(
      reinterpret_cast<uint64_t>(buffer_start_) + offset);

  comp = std::chrono::system_clock::now();

  compute();

  end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> elapsed_seconds = comp - start;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - comp;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

void ASMBackend::Compile() { rt_.add(&buffer_start_, &code_); }

void* ASMBackend::GetFunction(std::string_view name) const {
  auto label = public_fns_.at(name);
  auto offset = code_.labelOffsetFromBase(label);
  return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(buffer_start_) +
                                 offset);
}

}  // namespace kush::khir