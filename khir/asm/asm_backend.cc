#include "khir/asm/asm_backend.h"

#include <iostream>
#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "khir/asm/dfs_label.h"
#include "khir/asm/linear_scan_reg_alloc.h"
#include "khir/asm/materialize_gep.h"
#include "khir/asm/register_assignment.h"
#include "khir/asm/stack_spill_reg_alloc.h"
#include "khir/backend.h"
#include "khir/instruction.h"
#include "khir/opcode.h"
#include "khir/type_manager.h"
#include "util/profile_map_generator.h"

namespace kush::khir {

StackSlotAllocator::StackSlotAllocator(int32_t initial_size)
    : size_(initial_size) {}

int32_t StackSlotAllocator::AllocateSlot(int32_t bytes) {
  size_ += bytes;
  return -size_;
}

int32_t StackSlotAllocator::AllocateSlot(int32_t bytes, int32_t align) {
  if (size_ % align != 0) {
    size_ += align - (size_ % align);
  }
  size_ += bytes;
  return -size_;
}

int32_t StackSlotAllocator::GetSize() { return size_; }

using namespace asmjit;

void ExceptionErrorHandler::handleError(Error err, const char* message,
                                        BaseEmitter* origin) {
  throw std::runtime_error(message);
}

ASMBackend::ASMBackend(RegAllocImpl impl)
    : reg_alloc_impl_(impl), logger_(stderr) {}

uint64_t ASMBackend::OutputConstant(uint64_t instr, const Program& program) {
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
      asm_->embedUInt64(program.I64Constants()[i64_id]);
      return 8;
    }

    case ConstantOpcode::PTR_CONST: {
      auto ptr_id = Type1InstructionReader(instr).Constant();
      asm_->embedUInt64(
          reinterpret_cast<uint64_t>(program.PtrConstants()[ptr_id]));
      return 8;
    }

    case ConstantOpcode::F64_CONST: {
      auto f64_id = Type1InstructionReader(instr).Constant();
      asm_->embedDouble(program.F64Constants()[f64_id]);
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

    case ConstantOpcode::PTR_CAST: {
      khir::Value v(Type3InstructionReader(instr).Arg());
      return OutputConstant(program.ConstantInstrs()[v.GetIdx()], program);
    }

    case ConstantOpcode::FUNC_PTR: {
      asm_->embedLabel(
          internal_func_labels_[Type3InstructionReader(instr).Arg()]);
      return 8;
    }

    case ConstantOpcode::STRUCT_CONST: {
      auto struct_id = Type1InstructionReader(instr).Constant();
      const auto& struct_const = program.StructConstants()[struct_id];

      auto field_offsets =
          program.TypeManager().GetStructFieldOffsets(struct_const.Type());
      auto type_size = program.TypeManager().GetTypeSize(struct_const.Type());
      auto field_constants = struct_const.Fields();

      uint64_t bytes_written = 0;

      for (int i = 0; i < field_constants.size(); i++) {
        auto field_const = field_constants[i];

        // add any padding until we're at the offset
        while (bytes_written < field_offsets[i]) {
          asm_->embedUInt8(0);
          bytes_written++;
        }

        auto field_const_instr = program.ConstantInstrs()[field_const.GetIdx()];

        bytes_written += OutputConstant(field_const_instr, program);
      }

      while (bytes_written < type_size) {
        asm_->embedUInt8(0);
        bytes_written++;
      }

      return bytes_written;
    }

    case ConstantOpcode::ARRAY_CONST: {
      auto arr_id = Type1InstructionReader(instr).Constant();
      const auto& arr_const = program.ArrayConstants()[arr_id];

      uint64_t bytes_written = 0;
      for (auto elem_const : arr_const.Elements()) {
        auto elem_const_instr = program.ConstantInstrs()[elem_const.GetIdx()];
        // arrays have no padding
        bytes_written += OutputConstant(elem_const_instr, program);
      }
      return bytes_written;
    }

    case ConstantOpcode::I32_CONST_VEC4:
    case ConstantOpcode::I32_CONST_VEC8:
      return 0;
  }
}

void ASMBackend::Translate(const Program& program) {
  code_.init(rt_.environment());
  // code_.setLogger(&logger_);
  asm_ = std::make_unique<x86::Assembler>(&code_);

  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);

  asm_->section(data_section_);

  // Declare all functions
  for (const auto& function : program.Functions()) {
    external_func_addr_.push_back(function.Addr());
    internal_func_labels_.push_back(asm_->newLabel());
  }

  // Write out all string constants
  for (const auto& str : program.CharArrayConstants()) {
    char_array_constants_.push_back(asm_->newLabel());
    asm_->bind(char_array_constants_.back());
    for (char c : str) {
      asm_->embedUInt8(static_cast<uint8_t>(c));
    }
    asm_->embedUInt8(0);
  }

  // Write out all global variables
  for (const auto& global : program.Globals()) {
    globals_.push_back(asm_->newLabel());
    asm_->bind(globals_.back());
    auto v = global.InitialValue();
    OutputConstant(program.ConstantInstrs()[v.GetIdx()], program);
  }

  asm_->section(text_section_);

  const std::vector<x86::Gpq> callee_saved_registers = {
      x86::rbx, x86::r12, x86::r13, x86::r14, x86::r15};

  // Translate all function bodies
  for (int func_idx = 0; func_idx < program.Functions().size(); func_idx++) {
    const auto& func = program.Functions()[func_idx];
    if (func.External()) {
      continue;
    }
    const auto& instructions = func.Instrs();
    const auto& basic_blocks = func.BasicBlocks();

    num_floating_point_args_ = 0;
    num_regular_args_ = 0;
    num_stack_args_ = 0;

    asm_->bind(internal_func_labels_[func_idx]);
    auto func_end_label = asm_->newLabel();
    function_start_end_[func.Name()] =
        std::make_pair(internal_func_labels_[func_idx], func_end_label);

    auto order_analysis = DFSLabel(basic_blocks);

    std::vector<int> order(basic_blocks.size());
    for (int i = 0; i < basic_blocks.size(); i++) {
      order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](int bb1, int bb2) -> bool {
      return order_analysis.postorder_label[bb2] <
             order_analysis.postorder_label[bb1];
    });

    // Determine if GEPs should be materialized
    auto gep_materialize = ComputeGEPMaterialize(func);

    std::vector<RegisterAssignment> register_assign;
    switch (reg_alloc_impl_) {
      case RegAllocImpl::STACK_SPILL:
        register_assign = StackSpillingRegisterAlloc(instructions);
        break;

      case RegAllocImpl::LINEAR_SCAN:
        register_assign = LinearScanRegisterAlloc(func, gep_materialize,
                                                  program.TypeManager());
        break;
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

    std::vector<Label> basic_blocks_impl;
    Label epilogue = asm_->newLabel();
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(asm_->newLabel());
    }

    std::vector<int32_t> value_offsets(instructions.size(), INT32_MAX);

    // initially, after rbp is all callee saved registers
    StackSlotAllocator static_stack_allocator(8 *
                                              callee_saved_registers.size());
    for (int k = 0; k < order.size(); k++) {
      int bb = order[k];
      int next_bb = k + 1 < order.size() ? order[k + 1] : -1;

      asm_->bind(basic_blocks_impl[bb]);

      for (const auto& [seg_start, seg_end] : basic_blocks[bb].Segments()) {
        for (int instr_idx = seg_start; instr_idx <= seg_end; instr_idx++) {
          TranslateInstr(
              program, func, program.TypeManager(), program.PtrConstants(),
              program.I64Constants(), program.F64Constants(), basic_blocks_impl,
              program.Functions(), epilogue, value_offsets, instructions,
              program.ConstantInstrs(), instr_idx, static_stack_allocator,
              register_assign, gep_materialize, next_bb);
        }
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
    asm_->bind(func_end_label);
  }
}

asmjit::Label ASMBackend::GetGlobalPointer(uint64_t instr) {
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
      throw std::runtime_error("Invalid global pointer.");
  }
}

bool ASMBackend::IsNullPtr(khir::Value v,
                           const std::vector<uint64_t>& constant_instrs) {
  if (!v.IsConstantGlobal()) {
    return false;
  }

  return ConstantOpcodeFrom(
             GenericInstructionReader(constant_instrs[v.GetIdx()]).Opcode()) ==
         ConstantOpcode::NULLPTR;
}

bool ASMBackend::IsConstantPtr(khir::Value v,
                               const std::vector<uint64_t>& constant_instrs) {
  if (!v.IsConstantGlobal()) {
    return false;
  }

  return ConstantOpcodeFrom(
             GenericInstructionReader(constant_instrs[v.GetIdx()]).Opcode()) ==
         ConstantOpcode::PTR_CONST;
}

khir::Value GetConstantCastedPtr(khir::Value v,
                                 const std::vector<uint64_t>& constant_instrs) {
  if (!v.IsConstantGlobal()) {
    throw std::runtime_error("Invalid constant casted ptr.");
  }
  return khir::Value(Type3InstructionReader(constant_instrs[v.GetIdx()]).Arg());
}

bool ASMBackend::IsConstantCastedPtr(
    khir::Value v, const std::vector<uint64_t>& constant_instrs) {
  if (!v.IsConstantGlobal()) {
    return false;
  }

  return ConstantOpcodeFrom(
             GenericInstructionReader(constant_instrs[v.GetIdx()]).Opcode()) ==
         ConstantOpcode::PTR_CAST;
}

bool IsStaticGEP(khir::Value v, const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }
  return OpcodeFrom(
             GenericInstructionReader(instructions[v.GetIdx()]).Opcode()) ==
         Opcode::GEP_STATIC;
}

bool IsDynamicGEP(khir::Value v, const std::vector<uint64_t>& instructions) {
  if (v.IsConstantGlobal()) {
    return false;
  }
  return OpcodeFrom(
             GenericInstructionReader(instructions[v.GetIdx()]).Opcode()) ==
         Opcode::GEP_DYNAMIC;
}

GEPStaticInfo StaticGEP(khir::Value v, const std::vector<uint64_t>& instrs,
                        const std::vector<uint64_t>& constant_instrs) {
  if (v.IsConstantGlobal() ||
      OpcodeFrom(GenericInstructionReader(instrs[v.GetIdx()]).Opcode()) !=
          Opcode::GEP_STATIC) {
    throw std::runtime_error("Invalid GEP_STATIC");
  }

  Type2InstructionReader reader(instrs[v.GetIdx() - 1]);
  if (OpcodeFrom(reader.Opcode()) != Opcode::GEP_STATIC_OFFSET) {
    throw std::runtime_error("Invalid GEP_STATIC_OFFSET");
  }

  auto ptr = khir::Value(reader.Arg0());

  auto constant_value = khir::Value(reader.Arg1());
  if (!constant_value.IsConstantGlobal()) {
    throw std::runtime_error("Invalid GEP_STATIC_OFFSET constant");
  }
  int32_t offset =
      Type1InstructionReader(constant_instrs[constant_value.GetIdx()])
          .Constant();

  return GEPStaticInfo{
      .ptr = ptr,
      .offset = offset,
  };
}

GEPDynamicInfo DynamicGEP(khir::Value v, const std::vector<uint64_t>& instrs,
                          const std::vector<uint64_t>& constant_instrs) {
  if (v.IsConstantGlobal() ||
      OpcodeFrom(GenericInstructionReader(instrs[v.GetIdx()]).Opcode()) !=
          Opcode::GEP_DYNAMIC) {
    throw std::runtime_error("Invalid GEP_DYNAMIC");
  }

  Type3InstructionReader reader1(instrs[v.GetIdx()]);
  Type2InstructionReader reader2(instrs[v.GetIdx() - 1]);
  if (OpcodeFrom(reader2.Opcode()) != Opcode::GEP_DYNAMIC_OFFSET) {
    throw std::runtime_error("Invalid GEP_DYNAMIC_OFFSET");
  }

  auto ptr = khir::Value(reader1.Arg());
  auto type_size = reader1.Sarg();
  auto index = khir::Value(reader2.Arg0());

  auto constant_value = khir::Value(reader2.Arg1());
  if (!constant_value.IsConstantGlobal()) {
    throw std::runtime_error("Invalid GEP_DYNAMIC_OFFSET constant");
  }
  int32_t offset =
      Type1InstructionReader(constant_instrs[constant_value.GetIdx()])
          .Constant();

  return GEPDynamicInfo{
      .ptr = ptr,
      .index = index,
      .offset = offset,
      .type_size = type_size,
  };
}

int32_t GetOffset(const std::vector<int32_t>& offsets, int i) {
  auto offset = offsets[i];
  if (offset == INT32_MAX) {
    throw std::runtime_error("Undefined offset.");
  }

  return offset;
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

bool IsIFlag(const RegisterAssignment& reg) {
  return reg.IsRegister() && FRegister::IsIFlag(reg.Register());
}

bool IsFFlag(const RegisterAssignment& reg) {
  return reg.IsRegister() && FRegister::IsFFlag(reg.Register());
}

template <typename Dest>
void ASMBackend::StoreCmpFlags(Opcode opcode, Dest d) {
  switch (opcode) {
    case Opcode::PTR_CMP_NULLPTR:
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
    case Opcode::I64_CMP_NE:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetB()) {
      asm_->mov(dest, v_reg.GetB());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetB());
    } else {
      asm_->mov(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AndByteValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->and_(dest, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->and_(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movzx(GPRegister::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->and_(dest, GPRegister::RAX.GetB());
    } else {
      asm_->and_(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::OrByteValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    asm_->or_(dest, c8);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->or_(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movzx(GPRegister::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->or_(dest, GPRegister::RAX.GetB());
    } else {
      asm_->or_(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, GPRegister::RAX.GetB());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetB());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, GPRegister::RAX.GetB());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    asm_->mov(GPRegister::RAX.GetD(), c32);
    return GPRegister::RAX.GetB();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetB();
  } else {
    asm_->movsx(GPRegister::RAX.GetD(),
                x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return GPRegister::RAX.GetB();
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->movzx(dest, v_reg.GetB());
  } else {
    asm_->movzx(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::SextByteValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int8_t c8 = Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    int64_t c64 = c8;
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->movsx(dest, v_reg.GetB());
  } else {
    asm_->movsx(dest, x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

constexpr int BYTE_PTR_SIZE = 1;
constexpr int WORD_PTR_SIZE = 2;
constexpr int DWORD_PTR_SIZE = 4;
constexpr int QWORD_PTR_SIZE = 8;
// constexpr int XMMWORD_PTR_SIZE = 16;
constexpr int YMMWORD_PTR_SIZE = 32;

uint8_t GetShift(int32_t type_size) {
  switch (type_size) {
    case 1:
      return 0;

    case 2:
      return 1;

    case 4:
      return 2;

    case 8:
      return 3;

    default:
      throw std::runtime_error("Invalid type size");
  }
}

x86::Mem ASMBackend::GetDynamicGEPPtrValue(
    GEPDynamicInfo info, int32_t size, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  bool index_is_reg = register_assign[info.index.GetIdx()].IsRegister();

  if (info.ptr.IsConstantGlobal()) {
    while (IsConstantCastedPtr(info.ptr, constant_instrs)) {
      info.ptr = GetConstantCastedPtr(info.ptr, constant_instrs);
    }

    x86::Gpq index_reg;
    if (index_is_reg) {
      index_reg =
          GPRegister::FromId(register_assign[info.index.GetIdx()].Register())
              .GetQ();
    } else {
      index_reg = GPRegister::RAX.GetQ();
      asm_->mov(
          GPRegister::RAX.GetD(),
          x86::dword_ptr(x86::rbp, GetOffset(offsets, info.index.GetIdx())));
    }

    if (IsNullPtr(info.ptr, constant_instrs)) {
      return x86::ptr(info.offset, index_reg, GetShift(info.type_size), size);
    }

    if (IsConstantPtr(info.ptr, constant_instrs)) {
      uint64_t c64 = reinterpret_cast<uint64_t>(
          ptr_constants[Type1InstructionReader(
                            constant_instrs[info.ptr.GetIdx()])
                            .Constant()]);

      auto ptr_reg = GPRegister::RAX.GetQ();
      asm_->mov(ptr_reg, c64);
      return x86::ptr(ptr_reg, index_reg, GetShift(info.type_size), info.offset,
                      size);
    }

    auto label = GetGlobalPointer(constant_instrs[info.ptr.GetIdx()]);
    return x86::ptr(label, index_reg, GetShift(info.type_size), info.offset,
                    size);
  }

  bool ptr_is_reg = register_assign[info.ptr.GetIdx()].IsRegister();

  if (ptr_is_reg) {
    x86::Gpq ptr_reg =
        GPRegister::FromId(register_assign[info.ptr.GetIdx()].Register())
            .GetQ();

    x86::Gpq index_reg;
    if (index_is_reg) {
      index_reg =
          GPRegister::FromId(register_assign[info.index.GetIdx()].Register())
              .GetQ();
    } else {
      index_reg = GPRegister::RAX.GetQ();
      asm_->mov(
          GPRegister::RAX.GetD(),
          x86::dword_ptr(x86::rbp, GetOffset(offsets, info.index.GetIdx())));
    }

    return x86::ptr(ptr_reg, index_reg, GetShift(info.type_size), info.offset,
                    size);
  }

  // ptr is not a register
  if (index_is_reg) {
    x86::Gpq ptr_reg = GPRegister::RAX.GetQ();
    asm_->mov(ptr_reg,
              x86::qword_ptr(x86::rbp, GetOffset(offsets, info.ptr.GetIdx())));

    x86::Gpq index_reg =
        GPRegister::FromId(register_assign[info.index.GetIdx()].Register())
            .GetQ();

    return x86::ptr(ptr_reg, index_reg, GetShift(info.type_size), info.offset,
                    size);
  }

  // ptr is not a register
  // index is not a register

  // load index
  auto reg = GPRegister::RAX;
  asm_->mov(reg.GetD(),
            x86::dword_ptr(x86::rbp, GetOffset(offsets, info.index.GetIdx())));
  // shift by type_size, add offset
  asm_->shl(reg.GetQ(), GetShift(info.type_size));
  // add to ptr
  asm_->add(reg.GetQ(),
            x86::qword_ptr(x86::rbp, GetOffset(offsets, info.ptr.GetIdx())));
  return x86::ptr(reg.GetQ(), info.offset, size);
}

x86::Mem ASMBackend::GetStaticGEPPtrValue(
    GEPStaticInfo info, int32_t size, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (info.ptr.IsConstantGlobal()) {
    while (IsConstantCastedPtr(info.ptr, constant_instrs)) {
      info.ptr = GetConstantCastedPtr(info.ptr, constant_instrs);
    }

    if (IsNullPtr(info.ptr, constant_instrs)) {
      return x86::ptr(info.offset, size);
    }

    if (IsConstantPtr(info.ptr, constant_instrs)) {
      uint64_t c64 = reinterpret_cast<uint64_t>(
          ptr_constants[Type1InstructionReader(
                            constant_instrs[info.ptr.GetIdx()])
                            .Constant()]);
      auto ptr_reg = GPRegister::RAX.GetQ();
      asm_->mov(ptr_reg, c64);
      return x86::ptr(ptr_reg, info.offset, size);
    }

    auto label = GetGlobalPointer(constant_instrs[info.ptr.GetIdx()]);
    return x86::ptr(label, info.offset, size);
  }

  if (register_assign[info.ptr.GetIdx()].IsRegister()) {
    auto ptr_reg =
        GPRegister::FromId(register_assign[info.ptr.GetIdx()].Register())
            .GetQ();
    return x86::ptr(ptr_reg, info.offset, size);
  }

  auto ptr_reg = GPRegister::RAX.GetQ();
  asm_->mov(ptr_reg,
            x86::qword_ptr(x86::rbp, GetOffset(offsets, info.ptr.GetIdx())));
  return x86::ptr(ptr_reg, info.offset, size);
}

x86::Mem ASMBackend::GetBytePtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (IsDynamicGEP(v, instrs)) {
    auto info = DynamicGEP(v, instrs, constant_instrs);
    return GetDynamicGEPPtrValue(info, BYTE_PTR_SIZE, offsets, instrs,
                                 constant_instrs, ptr_constants,
                                 register_assign);
  }

  GEPStaticInfo info;
  if (IsStaticGEP(v, instrs)) {
    info = StaticGEP(v, instrs, constant_instrs);
  } else {
    info.ptr = v;
    info.offset = 0;
  }
  return GetStaticGEPPtrValue(info, BYTE_PTR_SIZE, offsets, instrs,
                              constant_instrs, ptr_constants, register_assign);
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetW()) {
      asm_->mov(dest, v_reg.GetW());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetW());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetW());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, GPRegister::RAX.GetW());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetW());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsx(GPRegister::RAX.GetD(),
                  x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, GPRegister::RAX.GetW());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    asm_->mov(GPRegister::RAX.GetD(), c32);
    return GPRegister::RAX.GetW();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetW();
  } else {
    asm_->movsx(GPRegister::RAX.GetD(),
                x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return GPRegister::RAX.GetW();
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->movzx(dest, v_reg.GetW());
  } else {
    asm_->movzx(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::SextWordValue(
    x86::Gpq dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int16_t c16 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    int64_t c64 = c16;
    asm_->mov(dest, c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->movsx(dest, v_reg.GetW());
  } else {
    asm_->movsx(dest, x86::word_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (IsDynamicGEP(v, instrs)) {
    auto info = DynamicGEP(v, instrs, constant_instrs);
    return GetDynamicGEPPtrValue(info, WORD_PTR_SIZE, offsets, instrs,
                                 constant_instrs, ptr_constants,
                                 register_assign);
  }

  GEPStaticInfo info;
  if (IsStaticGEP(v, instrs)) {
    info = StaticGEP(v, instrs, constant_instrs);
  } else {
    info.ptr = v;
    info.offset = 0;
  }
  return GetStaticGEPPtrValue(info, WORD_PTR_SIZE, offsets, instrs,
                              constant_instrs, ptr_constants, register_assign);
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetD()) {
      asm_->mov(dest, v_reg.GetD());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetD(),
                x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetD());
    } else {
      asm_->mov(dest, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetD());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetD(),
                x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, GPRegister::RAX.GetD());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetD());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetD(),
                x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, GPRegister::RAX.GetD());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    asm_->mov(GPRegister::RAX.GetD(), c32);
    return GPRegister::RAX.GetD();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetD();
  } else {
    asm_->mov(GPRegister::RAX.GetD(),
              x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return GPRegister::RAX.GetD();
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetD());
  } else {
    asm_->cmp(src, x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

void ASMBackend::ZextDWordValue(
    GPRegister dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    uint32_t c32 =
        Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
    uint64_t c64 = c32;
    asm_->mov(dest.GetQ(), c64);
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->mov(dest.GetD(), v_reg.GetD());
  } else {
    asm_->mov(dest.GetD(),
              x86::dword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetDWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (IsDynamicGEP(v, instrs)) {
    auto info = DynamicGEP(v, instrs, constant_instrs);
    return GetDynamicGEPPtrValue(info, DWORD_PTR_SIZE, offsets, instrs,
                                 constant_instrs, ptr_constants,
                                 register_assign);
  }

  GEPStaticInfo info;
  if (IsStaticGEP(v, instrs)) {
    info = StaticGEP(v, instrs, constant_instrs);
  } else {
    info.ptr = v;
    info.offset = 0;
  }
  return GetStaticGEPPtrValue(info, DWORD_PTR_SIZE, offsets, instrs,
                              constant_instrs, ptr_constants, register_assign);
}

x86::Mem ASMBackend::GetYMMWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (IsDynamicGEP(v, instrs)) {
    auto info = DynamicGEP(v, instrs, constant_instrs);
    return GetDynamicGEPPtrValue(info, YMMWORD_PTR_SIZE, offsets, instrs,
                                 constant_instrs, ptr_constants,
                                 register_assign);
  }

  GEPStaticInfo info;
  if (IsStaticGEP(v, instrs)) {
    info = StaticGEP(v, instrs, constant_instrs);
  } else {
    info.ptr = v;
    info.offset = 0;
  }
  return GetStaticGEPPtrValue(info, YMMWORD_PTR_SIZE, offsets, instrs,
                              constant_instrs, ptr_constants, register_assign);
}

template <typename T>
void ASMBackend::MovePtrValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    while (IsConstantCastedPtr(v, constant_instrs)) {
      v = GetConstantCastedPtr(v, constant_instrs);
    }

    if (IsNullPtr(v, constant_instrs)) {
      asm_->mov(dest, 0);
      return;
    } else if (IsConstantPtr(v, constant_instrs)) {
      uint64_t c64 = reinterpret_cast<uint64_t>(
          ptr_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                            .Constant()]);
      if constexpr (std::is_same_v<T, x86::Mem>) {
        asm_->mov(GPRegister::RAX.GetQ(), c64);
        asm_->mov(dest, GPRegister::RAX.GetQ());
      } else {
        asm_->mov(dest, c64);
      }
      return;
    }

    auto label = GetGlobalPointer(constant_instrs[v.GetIdx()]);
    asm_->lea(GPRegister::RAX.GetQ(), x86::ptr(label));
    asm_->mov(dest, GPRegister::RAX.GetQ());
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetQ()) {
      asm_->mov(dest, v_reg.GetQ());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetQ());
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
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(), c64);
      asm_->mov(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->mov(dest, c64);
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());

    // Coalesce
    if (dest != v_reg.GetQ()) {
      asm_->mov(dest, v_reg.GetQ());
    }
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetQ());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->add(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->add(dest, GPRegister::RAX.GetQ());
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->sub(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->sub(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->sub(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::AndQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(x86::rax, c64);
      asm_->and_(dest, x86::rax);
    } else {
      asm_->and_(dest, x86::qword_ptr(EmbedI64(c64)));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->and_(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->and_(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->and_(dest,
                 x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::XorQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(x86::rax, c64);
      asm_->xor_(dest, x86::rax);
    } else {
      asm_->xor_(dest, x86::qword_ptr(EmbedI64(c64)));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->xor_(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->xor_(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->xor_(dest,
                 x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::OrQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (v.IsConstantGlobal()) {
    int64_t c64 =
        i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                          .Constant()];

    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(x86::rax, c64);
      asm_->or_(dest, x86::rax);
    } else {
      asm_->or_(dest, x86::qword_ptr(EmbedI64(c64)));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->or_(dest, v_reg.GetQ());
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->mov(GPRegister::RAX.GetQ(),
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->or_(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->or_(dest, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

template <typename T>
void ASMBackend::LShiftQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (!v.IsConstantGlobal()) {
    throw std::runtime_error("Invalid LShift - must be constant");
  }

  int64_t c64 =
      i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                        .Constant()];

  if (c64 < 0 || c64 >= 64) {
    throw std::runtime_error("Shift constant must be in 0-63 range");
  }

  uint8_t c8 = c64;

  if constexpr (std::is_same_v<T, x86::Mem>) {
    asm_->shl(dest, c8);
  } else {
    asm_->shl(dest, c8);
  }
}

template <typename T>
void ASMBackend::RShiftQWordValue(
    T dest, Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (!v.IsConstantGlobal()) {
    throw std::runtime_error("Invalid LShift - must be constant");
  }

  int64_t c64 =
      i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                        .Constant()];

  if (c64 < 0 || c64 >= 64) {
    throw std::runtime_error("Shift constant must be in 0-63 range");
  }

  uint8_t c8 = c64;

  if constexpr (std::is_same_v<T, x86::Mem>) {
    asm_->shr(dest, c8);
  } else {
    asm_->shr(dest, c8);
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
    asm_->mov(GPRegister::RAX.GetQ(), c64);
    return GPRegister::RAX.GetQ();
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetQ();
  } else {
    asm_->mov(GPRegister::RAX.GetQ(),
              x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return GPRegister::RAX.GetQ();
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
    auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
    asm_->cmp(src, v_reg.GetQ());
  } else {
    asm_->cmp(src, x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
  }
}

x86::Mem ASMBackend::GetQWordPtrValue(
    Value v, std::vector<int32_t>& offsets, const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  if (IsDynamicGEP(v, instrs)) {
    auto info = DynamicGEP(v, instrs, constant_instrs);
    return GetDynamicGEPPtrValue(info, QWORD_PTR_SIZE, offsets, instrs,
                                 constant_instrs, ptr_constants,
                                 register_assign);
  }

  GEPStaticInfo info;
  if (IsStaticGEP(v, instrs)) {
    info = StaticGEP(v, instrs, constant_instrs);
  } else {
    info.ptr = v;
    info.offset = 0;
  }
  return GetStaticGEPPtrValue(info, QWORD_PTR_SIZE, offsets, instrs,
                              constant_instrs, ptr_constants, register_assign);
}

void ASMBackend::MaterializeGep(
    x86::Mem dest, khir::Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  auto reg = GPRegister::RAX.GetQ();
  asm_->lea(reg, GetQWordPtrValue(v, offsets, instrs, constant_instrs,
                                  ptr_constants, register_assign));
  asm_->mov(dest, reg);
}

void ASMBackend::MaterializeGep(
    x86::Gpq dest, khir::Value v, std::vector<int32_t>& offsets,
    const std::vector<uint64_t>& instrs,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<void*>& ptr_constants,
    const std::vector<RegisterAssignment>& register_assign) {
  asm_->lea(dest, GetQWordPtrValue(v, offsets, instrs, constant_instrs,
                                   ptr_constants, register_assign));
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
      asm_->movsd(x86::xmm15, x86::qword_ptr(label));
      asm_->movsd(dest, x86::xmm15);
    } else {
      asm_->movsd(dest, x86::qword_ptr(label));
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    auto reg = VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
    asm_->movsd(dest, reg);
  } else {
    if constexpr (std::is_same_v<T, x86::Mem>) {
      asm_->movsd(x86::xmm15,
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->movsd(dest, x86::xmm15);
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
    auto reg = VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
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
    auto reg = VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
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
    auto reg = VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
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
    auto reg = VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
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
      asm_->setp(dest);
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
    asm_->movsd(x86::xmm15, x86::qword_ptr(label));
    return x86::xmm15;
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    return VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX();
  } else {
    asm_->movsd(x86::xmm15,
                x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    return x86::xmm15;
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
    asm_->ucomisd(
        src, VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX());
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
        asm_->mov(GPRegister::RAX.GetQ(), c64);
        asm_->mov(dest, GPRegister::RAX.GetQ());
      }
    } else {
      asm_->mov(dest, c64);
    }
  } else if (register_assign[v.GetIdx()].IsRegister()) {
    if constexpr (std::is_same_v<x86::Mem, T>) {
      asm_->cvttsd2si(
          GPRegister::RAX.GetQ(),
          VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX());
      asm_->mov(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->cvttsd2si(
          dest,
          VRegister::FromId(register_assign[v.GetIdx()].Register()).GetX());
    }
  } else {
    if constexpr (std::is_same_v<x86::Mem, T>) {
      asm_->cvttsd2si(GPRegister::RAX.GetQ(),
                      x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
      asm_->mov(dest, GPRegister::RAX.GetQ());
    } else {
      asm_->cvttsd2si(dest,
                      x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
    }
  }
}

void ASMBackend::CondBrFlag(
    Value v, int true_bb, int false_bb,
    const std::vector<uint64_t>& instructions,
    const std::vector<RegisterAssignment>& register_assign,
    const std::vector<Label>& basic_blocks, int next_bb) {
  assert(!v.IsConstantGlobal());
  GenericInstructionReader reader(instructions[v.GetIdx()]);
  auto opcode = OpcodeFrom(reader.Opcode());

  auto tr = basic_blocks[true_bb];
  auto fl = basic_blocks[false_bb];

  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ: {
      if (true_bb == next_bb) {
        asm_->jne(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->je(tr);
        // fall through to the false_bb
      } else {
        asm_->je(tr);
        asm_->jmp(fl);
      }
      return;
    }

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
      if (true_bb == next_bb) {
        asm_->je(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->jne(tr);
        // fall through to the false_bb
      } else {
        asm_->jne(tr);
        asm_->jmp(fl);
      }
      return;
    }

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT: {
      if (true_bb == next_bb) {
        asm_->jge(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->jl(tr);
        // fall through to the false_bb
      } else {
        asm_->jl(tr);
        asm_->jmp(fl);
      }
      return;
    }

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE: {
      if (true_bb == next_bb) {
        asm_->jg(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->jle(tr);
        // fall through to the false_bb
      } else {
        asm_->jle(tr);
        asm_->jmp(fl);
      }
      return;
    }

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT: {
      if (true_bb == next_bb) {
        asm_->jle(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->jg(tr);
        // fall through to the false_bb
      } else {
        asm_->jg(tr);
        asm_->jmp(fl);
      }
      return;
    }

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE: {
      if (true_bb == next_bb) {
        asm_->jl(fl);
        // fall through to the true_bb
      } else if (false_bb == next_bb) {
        asm_->jge(tr);
        // fall through to the false_bb
      } else {
        asm_->jge(tr);
        asm_->jmp(fl);
      }
      return;
    }

    default:
      throw std::runtime_error("Not possible");
  }
}

void ASMBackend::CondBrF64Flag(
    Value v, int true_bb, int false_bb,
    const std::vector<uint64_t>& instructions,
    const std::vector<RegisterAssignment>& register_assign,
    const std::vector<Label>& basic_blocks, int next_bb) {
  assert(!v.IsConstantGlobal());
  GenericInstructionReader reader(instructions[v.GetIdx()]);
  auto opcode = OpcodeFrom(reader.Opcode());

  auto tr = basic_blocks[true_bb];
  auto fl = basic_blocks[false_bb];

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
    const Program& program, const Function& current_function,
    const TypeManager& type_manager, const std::vector<void*>& ptr_constants,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<Label>& basic_blocks,
    const std::vector<Function>& functions, const Label& epilogue,
    std::vector<int32_t>& offsets, const std::vector<uint64_t>& instructions,
    const std::vector<uint64_t>& constant_instrs, int instr_idx,
    StackSlotAllocator& stack_allocator,
    const std::vector<RegisterAssignment>& register_assign,
    const std::vector<bool>& gep_materialize, int next_bb) {
  auto instr = instructions[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());
  const auto& dest_assign = register_assign[instr_idx];

  switch (opcode) {
    // I1
    // ======================================================================
    case Opcode::I1_ZEXT_I8: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
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

    case Opcode::I1_AND: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        AndByteValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        AndByteValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    case Opcode::I1_OR: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        OrByteValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::byte_ptr(x86::rbp, offset);
        MoveByteValue(dest, v0, offsets, constant_instrs, register_assign);
        OrByteValue(dest, v1, offsets, constant_instrs, register_assign);
      }
      return;
    }

    // I8 ======================================================================
    case Opcode::I8_ADD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetB();
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

      MoveByteValue(GPRegister::RAX.GetB(), v0, offsets, constant_instrs,
                    register_assign);
      MulByteValue(v1, offsets, constant_instrs, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->movsx(GPRegister::FromId(dest_assign.Register()).GetD(),
                    GPRegister::RAX.GetB());
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::byte_ptr(x86::rbp, offset), GPRegister::RAX.GetB());
      }
      return;
    }

    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      // embed the constant array
      auto vec4_idx =
          Type1InstructionReader(program.ConstantInstrs()[v1.GetIdx()])
              .Constant();
      const auto& vec4 = program.I32Vec4Constants()[vec4_idx];
      auto label = asm_->newLabel();
      asm_->section(data_section_);
      asm_->align(AlignMode::kAlignZero, 16);
      asm_->bind(label);
      for (int32_t v : vec4) {
        asm_->embedInt32(v);
      }
      asm_->section(text_section_);

      // load the value into xmm15
      if (register_assign[v0.GetIdx()].IsRegister()) {
        asm_->vmovd(
            x86::xmm15,
            GPRegister::FromId(register_assign[v0.GetIdx()].Register()).GetD());
      } else {
        asm_->vmovd(x86::xmm15,
                    x86::dword_ptr(x86::rbp, GetOffset(offsets, v0.GetIdx())));
      }
      // compare to each value
      asm_->vpbroadcastd(x86::xmm15, x86::xmm15);
      asm_->vpcmpeqd(x86::xmm15, x86::xmm15, x86::xmmword_ptr(label));
      asm_->vmovmskps(x86::eax, x86::xmm15);
      asm_->test(x86::eax, x86::eax);

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      // embed the constant array
      auto vec8_idx =
          Type1InstructionReader(program.ConstantInstrs()[v1.GetIdx()])
              .Constant();
      const auto& vec8 = program.I32Vec8Constants()[vec8_idx];
      auto label = asm_->newLabel();
      asm_->section(data_section_);
      asm_->align(AlignMode::kAlignZero, 32);
      asm_->bind(label);
      for (int32_t v : vec8) {
        asm_->embedInt32(v);
      }
      asm_->section(text_section_);

      // load the value into xmm15
      if (register_assign[v0.GetIdx()].IsRegister()) {
        asm_->vmovd(
            x86::xmm15,
            GPRegister::FromId(register_assign[v0.GetIdx()].Register()).GetD());
      } else {
        asm_->vmovd(x86::xmm15,
                    x86::dword_ptr(x86::rbp, GetOffset(offsets, v0.GetIdx())));
      }
      // duplicate
      asm_->vpbroadcastd(x86::ymm7, x86::xmm15);

      // compare to each value
      asm_->vpcmpeqd(x86::ymm7, x86::ymm7, x86::ymmword_ptr(label));
      asm_->vmovmskps(x86::eax, x86::ymm7);
      asm_->vzeroupper();
      asm_->test(x86::eax, x86::eax);

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
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

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
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
                      ? GPRegister::FromId(dest_assign.Register())
                      : GPRegister::RAX;
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

      auto dest = dest_assign.IsRegister()
                      ? VRegister::FromId(dest_assign.Register()).GetX()
                      : x86::xmm15;

      if (v.IsConstantGlobal()) {
        int8_t c8 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c8;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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

    case Opcode::I1_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetBytePtrValue(v, offsets, instructions, constant_instrs,
                                 ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        if (IsIFlag(dest_assign)) {
          throw std::runtime_error("Cannot allocate the flag reg to I1 LOAD");
        }

        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetB(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movzx(GPRegister::RAX.GetQ(), loc);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), GPRegister::RAX.GetB());
      }
      return;
    }

    case Opcode::I8_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetBytePtrValue(v, offsets, instructions, constant_instrs,
                                 ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetB(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movzx(GPRegister::RAX.GetQ(), loc);
        asm_->mov(x86::byte_ptr(x86::rbp, offset), GPRegister::RAX.GetB());
      }
      return;
    }

    case Opcode::I8_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetBytePtrValue(v0, offsets, instructions, constant_instrs,
                                 ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        int8_t c8 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c8);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, GPRegister::FromId(v1_reg).GetB());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = GPRegister::FromId(dest_assign.Register());
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetW();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetW();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetW();
        MoveWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = GPRegister::RAX.GetW();
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

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
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
                      ? GPRegister::FromId(dest_assign.Register())
                      : GPRegister::RAX;
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

      auto dest = dest_assign.IsRegister()
                      ? VRegister::FromId(dest_assign.Register()).GetX()
                      : x86::xmm15;

      if (v.IsConstantGlobal()) {
        int16_t c16 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c16;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
                                 ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetW(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movzx(GPRegister::RAX.GetQ(), loc);
        asm_->mov(x86::word_ptr(x86::rbp, offset), GPRegister::RAX.GetW());
      }
      return;
    }

    case Opcode::I16_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetWordPtrValue(v0, offsets, instructions, constant_instrs,
                                 ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        int16_t c16 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c16);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, GPRegister::FromId(v1_reg).GetW());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = GPRegister::FromId(dest_assign.Register());
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetD();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetD();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetD();
        MoveDWordValue(dest, v0, offsets, constant_instrs, register_assign);
        MulDWordValue(dest, v1, offsets, constant_instrs, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = GPRegister::RAX.GetD();
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

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
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
                      ? GPRegister::FromId(dest_assign.Register())
                      : GPRegister::RAX;
      ZextDWordValue(dest, v, offsets, constant_instrs, register_assign);

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

      auto dest = dest_assign.IsRegister()
                      ? VRegister::FromId(dest_assign.Register()).GetX()
                      : x86::xmm15;

      if (v.IsConstantGlobal()) {
        int32_t c32 =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();
        double c64 = c32;
        auto label = EmbedF64(c64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
                                  ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetD(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(GPRegister::RAX.GetD(), loc);
        asm_->mov(x86::dword_ptr(x86::rbp, offset), GPRegister::RAX.GetD());
      }
      return;
    }

    case Opcode::I32_VEC8_LOAD: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto loc = GetYMMWordPtrValue(v, offsets, instructions, constant_instrs,
                                    ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->vmovaps(VRegister::FromId(dest_assign.Register()).GetY(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot(32, 32);
        offsets[instr_idx] = offset;
        asm_->vmovaps(x86::ymm15, loc);
        asm_->vmovaps(x86::ymmword_ptr(x86::rbp, offset), x86::ymm15);
      }
      return;
    }

    case Opcode::I32_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetDWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        int32_t c32 =
            Type1InstructionReader(constant_instrs[v1.GetIdx()]).Constant();
        asm_->mov(loc, c32);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, GPRegister::FromId(v1_reg).GetD());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = GPRegister::FromId(dest_assign.Register());
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
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
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
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

    case Opcode::I64_AND: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        AndQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        AndQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      }
      return;
    }

    case Opcode::I64_XOR: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        XorQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        XorQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      }
      return;
    }

    case Opcode::I64_OR: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        OrQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                     register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        OrQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                     register_assign);
      }
      return;
    }

    case Opcode::I64_LSHIFT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        LShiftQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                         register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        LShiftQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                         register_assign);
      }
      return;
    }

    case Opcode::I64_RSHIFT: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        RShiftQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                         register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = x86::qword_ptr(x86::rbp, offset);
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        RShiftQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                         register_assign);
      }
      return;
    }

    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = GPRegister::FromId(dest_assign.Register()).GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        MulQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto dest = GPRegister::RAX.GetQ();
        MoveQWordValue(dest, v0, offsets, constant_instrs, i64_constants,
                       register_assign);
        MulQWordValue(dest, v1, offsets, constant_instrs, i64_constants,
                      register_assign);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), dest);
      }
      return;
    }

    case Opcode::PTR_CMP_NULLPTR: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      MovePtrValue(GPRegister::RAX.GetQ(), v, offsets, constant_instrs,
                   ptr_constants, register_assign);
      asm_->cmp(GPRegister::RAX.GetQ(), 0);

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
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

      auto v0_reg = GetQWordValue(v0, offsets, constant_instrs, i64_constants,
                                  register_assign);
      CmpQWordValue(v0_reg, v1, offsets, constant_instrs, i64_constants,
                    register_assign);

      if (IsIFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
        StoreCmpFlags(opcode, loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        auto loc = x86::byte_ptr(x86::rbp, offset);
        StoreCmpFlags(opcode, loc);
      }
      return;
    }

    case Opcode::I64_TRUNC_I16: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? GPRegister::FromId(dest_assign.Register())
                      : GPRegister::RAX;
      MoveQWordValue(dest.GetQ(), v, offsets, constant_instrs, i64_constants,
                     register_assign);

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::word_ptr(x86::rbp, offset), dest.GetW());
      }
      return;
    }

    case Opcode::I64_TRUNC_I32: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? GPRegister::FromId(dest_assign.Register())
                      : GPRegister::RAX;
      MoveQWordValue(dest.GetQ(), v, offsets, constant_instrs, i64_constants,
                     register_assign);

      if (!dest_assign.IsRegister()) {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::dword_ptr(x86::rbp, offset), dest.GetD());
      }
      return;
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      Value v(reader.Arg0());

      auto dest = dest_assign.IsRegister()
                      ? VRegister::FromId(dest_assign.Register()).GetX()
                      : x86::xmm15;

      if (v.IsConstantGlobal()) {
        int64_t ci64 =
            i64_constants[Type1InstructionReader(constant_instrs[v.GetIdx()])
                              .Constant()];
        double cf64 = ci64;
        auto label = EmbedF64(cf64);
        asm_->movsd(dest, x86::qword_ptr(label));
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
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
                                  ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        assert(dest_assign.IsRegister());
        int64_t ci64 =
            i64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        auto dest = GPRegister::FromId(dest_assign.Register());

        if (ci64 >= INT32_MIN && ci64 <= INT32_MAX) {
          int32_t ci32 = ci64;
          asm_->mov(loc, ci32);
        } else {
          asm_->mov(dest.GetQ(), ci64);
          asm_->mov(loc, dest.GetQ());
        }
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, GPRegister::FromId(v1_reg).GetQ());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = GPRegister::FromId(dest_assign.Register());
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
                                  ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetQ(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(GPRegister::RAX.GetQ(), loc);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), GPRegister::RAX.GetQ());
      }
      return;
    }

    // PTR =====================================================================
    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);

      auto ptr_type = static_cast<khir::Type>(reader.TypeID());
      auto type = type_manager.GetPointerElementType(ptr_type);
      int num_values = reader.Arg();
      auto size = type_manager.GetTypeSize(type) * num_values;
      size += (16 - (size % 16)) % 16;
      assert(size % 16 == 0);

      asm_->sub(x86::rsp, size);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetQ(), x86::rsp);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(x86::qword_ptr(x86::rbp, offset), x86::rsp);
      }
      return;
    }

    case Opcode::GEP_DYNAMIC_OFFSET:
    case Opcode::GEP_STATIC_OFFSET: {
      // do nothing since it is an auxiliary instruction
      return;
    }

    case Opcode::GEP_DYNAMIC:
    case Opcode::GEP_STATIC: {
      if (!gep_materialize[instr_idx]) {
        return;
      }

      Type3InstructionReader reader(instr);
      Value v(instr_idx);

      if (dest_assign.IsRegister()) {
        auto dest_reg = GPRegister::FromId(dest_assign.Register()).GetQ();
        MaterializeGep(dest_reg, v, offsets, instructions, constant_instrs,
                       ptr_constants, register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MaterializeGep(x86::qword_ptr(x86::rbp, offset), v, offsets,
                       instructions, constant_instrs, ptr_constants,
                       register_assign);
      }
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());

      if (dest_assign.IsRegister()) {
        auto dest_reg = GPRegister::FromId(dest_assign.Register()).GetQ();

        if (v.IsConstantGlobal()) {
          throw std::runtime_error("Expected non-const PTR");
        } else if (register_assign[v.GetIdx()].IsRegister()) {
          auto v_reg =
              GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetQ();
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
        throw std::runtime_error("Expected non-const PTR");
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        auto v_reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
        asm_->mov(x86::qword_ptr(x86::rbp, offset), v_reg.GetQ());
      } else {
        asm_->mov(GPRegister::RAX.GetQ(),
                  x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        asm_->mov(x86::qword_ptr(x86::rbp, offset), GPRegister::RAX.GetQ());
      }
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      Value v(reader.Arg());

      auto loc = GetQWordPtrValue(v, offsets, instructions, constant_instrs,
                                  ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->mov(GPRegister::FromId(dest_assign.Register()).GetQ(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->mov(GPRegister::RAX.GetQ(), loc);
        asm_->mov(x86::qword_ptr(x86::rbp, offset), GPRegister::RAX.GetQ());
      }
      return;
    }

    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        assert(dest_assign.IsRegister());

        while (IsConstantCastedPtr(v1, constant_instrs)) {
          v1 = GetConstantCastedPtr(v1, constant_instrs);
        }

        if (IsNullPtr(v1, constant_instrs)) {
          asm_->mov(loc, 0);
          return;
        } else if (IsConstantPtr(v1, constant_instrs)) {
          auto dest = GPRegister::FromId(dest_assign.Register());
          uint64_t c64 = reinterpret_cast<uint64_t>(
              ptr_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                                .Constant()]);
          asm_->mov(dest.GetQ(), c64);
          asm_->mov(loc, dest.GetQ());
          return;
        }

        auto label = GetGlobalPointer(constant_instrs[v1.GetIdx()]);
        auto dest = GPRegister::FromId(dest_assign.Register());
        asm_->lea(dest.GetQ(), x86::ptr(label));
        asm_->mov(loc, dest.GetQ());
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg = register_assign[v1.GetIdx()].Register();
        asm_->mov(loc, GPRegister::FromId(v1_reg).GetQ());
      } else {
        // STORE operations from spilled operands mem need an extra scratch
        // register
        assert(dest_assign.IsRegister());
        auto dest = GPRegister::FromId(dest_assign.Register());
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
        auto dest = VRegister::FromId(dest_assign.Register()).GetX();
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        AddF64Value(dest, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm15, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        AddF64Value(x86::xmm15, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm15);
      }
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = VRegister::FromId(dest_assign.Register()).GetX();
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        SubF64Value(dest, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm15, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        SubF64Value(x86::xmm15, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm15);
      }
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = VRegister::FromId(dest_assign.Register()).GetX();
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        MulF64Value(dest, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm15, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        MulF64Value(x86::xmm15, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm15);
      }
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());
      Value v1(reader.Arg1());

      if (dest_assign.IsRegister()) {
        auto dest = VRegister::FromId(dest_assign.Register()).GetX();
        MoveF64Value(dest, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        DivF64Value(dest, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        MoveF64Value(x86::xmm15, v0, offsets, constant_instrs, f64_constants,
                     register_assign);
        DivF64Value(x86::xmm15, v1, offsets, constant_instrs, f64_constants,
                    register_assign);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm15);
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

      if (IsFFlag(dest_assign)) {
        return;
      }

      if (dest_assign.IsRegister()) {
        auto loc = GPRegister::FromId(dest_assign.Register()).GetB();
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
        auto loc = GPRegister::FromId(dest_assign.Register()).GetQ();
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
                                  ptr_constants, register_assign);

      if (v1.IsConstantGlobal()) {
        double cf64 =
            f64_constants[Type1InstructionReader(constant_instrs[v1.GetIdx()])
                              .Constant()];
        asm_->movsd(x86::xmm15, x86::qword_ptr(EmbedF64(cf64)));
        asm_->movsd(loc, x86::xmm15);
      } else if (register_assign[v1.GetIdx()].IsRegister()) {
        auto v1_reg =
            VRegister::FromId(register_assign[v1.GetIdx()].Register()).GetX();
        asm_->movsd(loc, v1_reg);
      } else {
        asm_->movsd(x86::xmm15,
                    x86::qword_ptr(x86::rbp, GetOffset(offsets, v1.GetIdx())));
        asm_->movsd(loc, x86::xmm15);
      }
      return;
    }

    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      Value v0(reader.Arg0());

      auto loc = GetQWordPtrValue(v0, offsets, instructions, constant_instrs,
                                  ptr_constants, register_assign);

      if (dest_assign.IsRegister()) {
        asm_->movsd(VRegister::FromId(dest_assign.Register()).GetX(), loc);
      } else {
        auto offset = stack_allocator.AllocateSlot();
        offsets[instr_idx] = offset;
        asm_->movsd(x86::xmm15, loc);
        asm_->movsd(x86::qword_ptr(x86::rbp, offset), x86::xmm15);
      }
      return;
    }

    // Control Flow ============================================================
    case Opcode::BR: {
      int dest_bb = Type5InstructionReader(instr).Marg0();
      if (dest_bb == next_bb) {
        // we can fall through to this
      } else {
        asm_->jmp(basic_blocks[dest_bb]);
      }
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      khir::Value v(reader.Arg());

      if (v.IsConstantGlobal()) {
        int8_t c =
            Type1InstructionReader(constant_instrs[v.GetIdx()]).Constant();

        int dest_bb;
        if (c != 0) {
          dest_bb = reader.Marg0();
        } else {
          dest_bb = reader.Marg1();
        }

        if (dest_bb == next_bb) {
          // we can fall through to this
        } else {
          asm_->jmp(basic_blocks[dest_bb]);
        }
        return;
      }

      int true_bb = reader.Marg0();
      int false_bb = reader.Marg1();

      if (IsIFlag(register_assign[v.GetIdx()])) {
        CondBrFlag(v, true_bb, false_bb, instructions, register_assign,
                   basic_blocks, next_bb);
      } else if (IsFFlag(register_assign[v.GetIdx()])) {
        CondBrF64Flag(v, true_bb, false_bb, instructions, register_assign,
                      basic_blocks, next_bb);
      } else if (register_assign[v.GetIdx()].IsRegister()) {
        asm_->cmp(
            GPRegister::FromId(register_assign[v.GetIdx()].Register()).GetB(),
            0);

        if (next_bb == true_bb) {
          asm_->je(basic_blocks[false_bb]);
          // fall through to the true_bb
        } else if (next_bb == false_bb) {
          asm_->jne(basic_blocks[true_bb]);
          // fall through to the false
        } else {
          asm_->jne(basic_blocks[true_bb]);
          asm_->jmp(basic_blocks[false_bb]);
        }
      } else {
        asm_->cmp(x86::byte_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())), 0);
        if (next_bb == true_bb) {
          asm_->je(basic_blocks[false_bb]);
          // fall through to the true_bb
        } else if (next_bb == false_bb) {
          asm_->jne(basic_blocks[true_bb]);
          // fall through to the false
        } else {
          asm_->jne(basic_blocks[true_bb]);
          asm_->jmp(basic_blocks[false_bb]);
        }
      }
      return;
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      Value phi(reader.Arg0());
      Value v(reader.Arg1());
      auto type = static_cast<khir::Type>(
          Type3InstructionReader(instructions[phi.GetIdx()]).TypeID());

      const auto& dest_assign = register_assign[phi.GetIdx()];
      if (!dest_assign.IsRegister() && offsets[phi.GetIdx()] == INT32_MAX) {
        offsets[phi.GetIdx()] = stack_allocator.AllocateSlot();
      }

      if (type_manager.IsF64Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = VRegister::FromId(dest_assign.Register()).GetX();
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
          auto dst = GPRegister::FromId(dest_assign.Register()).GetB();
          MoveByteValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::byte_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveByteValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI16Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = GPRegister::FromId(dest_assign.Register()).GetW();
          MoveWordValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::word_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveWordValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI32Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = GPRegister::FromId(dest_assign.Register()).GetD();
          MoveDWordValue(dst, v, offsets, constant_instrs, register_assign);
        } else {
          auto dst = x86::dword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MoveDWordValue(dst, v, offsets, constant_instrs, register_assign);
        }
        return;
      }

      if (type_manager.IsI64Type(type)) {
        if (dest_assign.IsRegister()) {
          auto dst = GPRegister::FromId(dest_assign.Register()).GetQ();
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
          auto dst = GPRegister::FromId(dest_assign.Register()).GetQ();
          MovePtrValue(dst, v, offsets, constant_instrs, ptr_constants,
                       register_assign);
        } else {
          auto dst = x86::qword_ptr(x86::rbp, offsets[phi.GetIdx()]);
          MovePtrValue(dst, v, offsets, constant_instrs, ptr_constants,
                       register_assign);
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
      } else if (type_manager.IsI1Type(type) || type_manager.IsI8Type(type)) {
        MoveByteValue(GPRegister::RAX.GetB(), v, offsets, constant_instrs,
                      register_assign);
      } else if (type_manager.IsI16Type(type)) {
        MoveWordValue(GPRegister::RAX.GetW(), v, offsets, constant_instrs,
                      register_assign);
      } else if (type_manager.IsI32Type(type)) {
        MoveDWordValue(GPRegister::RAX.GetD(), v, offsets, constant_instrs,
                       register_assign);
      } else if (type_manager.IsI64Type(type)) {
        MoveQWordValue(GPRegister::RAX.GetQ(), v, offsets, constant_instrs,
                       i64_constants, register_assign);
      } else if (type_manager.IsPtrType(type)) {
        MovePtrValue(GPRegister::RAX.GetQ(), v, offsets, constant_instrs,
                     ptr_constants, register_assign);
      } else {
        throw std::runtime_error("Invalid return value type.");
      }

      asm_->jmp(epilogue);
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto type = static_cast<Type>(reader.TypeID());

      const std::vector<GPRegister> normal_arg_regs = {
          GPRegister::RDI, GPRegister::RSI, GPRegister::RDX,
          GPRegister::RCX, GPRegister::R8,  GPRegister::R9};
      const std::vector<x86::Xmm> fp_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3,
          x86::xmm4, x86::xmm5, x86::xmm6, x86::xmm7};

      if (num_floating_point_args_ >= fp_arg_regs.size()) {
        throw std::runtime_error("Unsupported. Too many floating point args.");
      }

      if (num_regular_args_ >= normal_arg_regs.size()) {
        throw std::runtime_error("Unsupported. Too many regular args.");
      }

      if (type_manager.IsF64Type(type)) {
        auto src = fp_arg_regs[num_floating_point_args_++];

        if (dest_assign.IsRegister()) {
          auto dst = VRegister::FromId(dest_assign.Register()).GetX();
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
          auto dst = GPRegister::FromId(dest_assign.Register());
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
          auto dst = GPRegister::FromId(dest_assign.Register());
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
          auto dst = GPRegister::FromId(dest_assign.Register());
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
          auto dst = GPRegister::FromId(dest_assign.Register());
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
      const std::vector<GPRegister> normal_arg_regs = {
          GPRegister::RDI, GPRegister::RSI, GPRegister::RDX,
          GPRegister::RCX, GPRegister::R8,  GPRegister::R9};
      const std::vector<x86::Xmm> float_arg_regs = {
          x86::xmm0, x86::xmm1, x86::xmm2, x86::xmm3,
          x86::xmm4, x86::xmm5, x86::xmm6, x86::xmm7};

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
          SextByteValue(reg.GetQ(), v, offsets, constant_instrs,
                        register_assign);
        } else if (type_manager.IsI16Type(type)) {
          SextWordValue(reg.GetQ(), v, offsets, constant_instrs,
                        register_assign);
        } else if (type_manager.IsI32Type(type)) {
          MoveDWordValue(reg.GetD(), v, offsets, constant_instrs,
                         register_assign);
        } else if (type_manager.IsI64Type(type)) {
          MoveQWordValue(reg.GetQ(), v, offsets, constant_instrs, i64_constants,
                         register_assign);
        } else if (type_manager.IsPtrType(type)) {
          MovePtrValue(reg.GetQ(), v, offsets, constant_instrs, ptr_constants,
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
            MovePtrValue(loc, v, offsets, constant_instrs, ptr_constants,
                         register_assign);
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
        return_type = type_manager.GetFunctionReturnType(func.Type());

      } else {
        Type3InstructionReader reader(instr);
        khir::Value v(reader.Arg());
        if (v.IsConstantGlobal()) {
          while (IsConstantCastedPtr(v, constant_instrs)) {
            v = GetConstantCastedPtr(v, constant_instrs);
          }

          if (IsConstantPtr(v, constant_instrs)) {
            uint64_t c64 = reinterpret_cast<uint64_t>(
                ptr_constants[Type1InstructionReader(
                                  constant_instrs[v.GetIdx()])
                                  .Constant()]);

            auto ptr_reg = GPRegister::RAX.GetQ();
            asm_->mov(ptr_reg, c64);
            asm_->call(ptr_reg);
          } else {
            throw std::runtime_error("Not possible");
          }
        } else if (register_assign[v.GetIdx()].IsRegister()) {
          auto reg = GPRegister::FromId(register_assign[v.GetIdx()].Register());
          asm_->call(reg.GetQ());
        } else {
          asm_->call(x86::qword_ptr(x86::rbp, GetOffset(offsets, v.GetIdx())));
        }

        return_type = static_cast<Type>(reader.TypeID());
      }

      if (dynamic_stack_alloc != 0) {
        asm_->add(x86::rsp, dynamic_stack_alloc);
      }

      if (!type_manager.IsVoid(return_type)) {
        if (type_manager.IsF64Type(return_type)) {
          if (dest_assign.IsRegister()) {
            auto reg = VRegister::FromId(dest_assign.Register()).GetX();
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
          auto reg = GPRegister::FromId(dest_assign.Register());
          if (reg.GetQ() != GPRegister::RAX.GetQ()) {
            asm_->mov(reg.GetQ(), GPRegister::RAX.GetQ());
          }
        } else {
          auto offset = stack_allocator.AllocateSlot();
          offsets[instr_idx] = offset;
          asm_->mov(x86::qword_ptr(x86::rbp, offset), GPRegister::RAX.GetQ());
        }
      }
      return;
    }
  }
}

void ASMBackend::Compile() {
  rt_.add(&buffer_start_, &code_);

#ifdef PROFILE_ENABLED
  for (const auto& [name, labels] : function_start_end_) {
    auto [begin_label, end_label] = labels;

    auto begin_offset = code_.labelOffsetFromBase(begin_label);
    auto end_offset = code_.labelOffsetFromBase(end_label);
    auto begin_addr = reinterpret_cast<void*>(
        reinterpret_cast<uint64_t>(buffer_start_) + begin_offset);
    auto code_size = end_offset - begin_offset;

    util::ProfileMapGenerator::Get().AddEntry(begin_addr, code_size, name);
  }
#endif
}

void* ASMBackend::GetFunction(std::string_view name) const {
  auto [begin_label, end_label] = function_start_end_.at(name);
  auto offset = code_.labelOffsetFromBase(begin_label);
  return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(buffer_start_) +
                                 offset);
}

}  // namespace kush::khir