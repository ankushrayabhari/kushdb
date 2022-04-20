#include "khir/program_builder.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

FunctionBuilder::FunctionBuilder(ProgramBuilder& program_builder,
                                 std::string_view name,
                                 khir::Type function_type,
                                 khir::Type result_type,
                                 absl::Span<const khir::Type> arg_types,
                                 bool external, bool p, void* func)
    : program_builder_(program_builder),
      name_(name),
      return_type_(result_type),
      arg_types_(arg_types.begin(), arg_types.end()),
      function_type_(function_type),
      func_(func),
      external_(external),
      public_(p) {}

void FunctionBuilder::InitBody() {
  current_basic_block_ = GenerateBasicBlock();
  for (int i = 0; i < arg_types_.size(); i++) {
    arg_values_.push_back(Append(Type3InstructionBuilder()
                                     .SetOpcode(OpcodeTo(Opcode::FUNC_ARG))
                                     .SetSarg(i)
                                     .SetTypeID(arg_types_[i].GetID())
                                     .Build()));
  }
}

absl::Span<const Value> FunctionBuilder::GetFunctionArguments() const {
  if (external_) {
    throw std::runtime_error(
        "Cannot get argument registers of external function");
  }

  return arg_values_;
}

khir::Type FunctionBuilder::ReturnType() const { return return_type_; }

khir::Type FunctionBuilder::Type() const { return function_type_; }

bool FunctionBuilder::External() const { return external_; }

bool FunctionBuilder::Public() const { return public_; }

void* FunctionBuilder::Addr() const { return func_; }

std::string_view FunctionBuilder::Name() const { return name_; }

const std::vector<int>& FunctionBuilder::BasicBlockOrder() const {
  return basic_block_order_;
}

const std::vector<std::pair<int, int>>& FunctionBuilder::BasicBlocks() const {
  return basic_blocks_;
}

const std::vector<std::vector<int>>& FunctionBuilder::BasicBlockSuccessors()
    const {
  return basic_block_successors_;
}

const std::vector<std::vector<int>>& FunctionBuilder::BasicBlockPredecessors()
    const {
  return basic_block_predecessors_;
}

const std::vector<uint64_t>& FunctionBuilder::Instructions() const {
  return instructions_;
}

bool IsTerminatingInstr(Opcode opcode) {
  switch (opcode) {
    case Opcode::BR:
    case Opcode::CONDBR:
    case Opcode::RETURN:
    case Opcode::RETURN_VALUE:
      return true;

    default:
      return false;
  }
}

void UpdateSuccessors(std::vector<std::vector<int>>& pred,
                      std::vector<std::vector<int>>& succ, int curr,
                      uint64_t instr) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());
  switch (opcode) {
    case Opcode::BR: {
      Type5InstructionReader reader(instr);
      succ[curr].push_back(reader.Marg0());
      pred[reader.Marg0()].push_back(curr);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      succ[curr].push_back(reader.Marg0());
      pred[reader.Marg0()].push_back(curr);
      succ[curr].push_back(reader.Marg1());
      pred[reader.Marg1()].push_back(curr);
      return;
    }

    default:
      return;
  }
}

Value FunctionBuilder::Append(uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot get add body to external function");
  }

  auto idx = instructions_.size();
  instructions_.push_back(instr);

  if (basic_blocks_[current_basic_block_].second >= 0) {
    throw std::runtime_error("Cannot append to terminated basic block.");
  }

  if (basic_blocks_[current_basic_block_].first < 0) {
    basic_blocks_[current_basic_block_].first = idx;
  }

  if (basic_blocks_[current_basic_block_].second < 0 &&
      IsTerminatingInstr(
          OpcodeFrom(GenericInstructionReader(instr).Opcode()))) {
    basic_blocks_[current_basic_block_].second = idx;
    UpdateSuccessors(basic_block_predecessors_, basic_block_successors_,
                     current_basic_block_, instr);
    basic_block_order_.push_back(current_basic_block_);
  }

  return Value(idx, false);
}

void FunctionBuilder::Update(Value pos, uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot mutate body of external function");
  }

  auto idx = pos.GetIdx();
  instructions_[idx] = instr;
}

uint64_t FunctionBuilder::GetInstruction(Value v) const {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  if (v.IsConstantGlobal()) {
    return program_builder_.GetConstantGlobalInstr(v);
  }

  return instructions_[v.GetIdx()];
}

int FunctionBuilder::GenerateBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  auto idx = basic_blocks_.size();
  basic_blocks_.push_back({-1, -1});
  basic_block_successors_.emplace_back();
  basic_block_predecessors_.emplace_back();
  return idx;
}

void FunctionBuilder::SetCurrentBasicBlock(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  current_basic_block_ = basic_block_id;
}
int FunctionBuilder::GetCurrentBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return current_basic_block_;
}

bool FunctionBuilder::IsTerminated(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return basic_blocks_[current_basic_block_].second >= 0;
}

Value ProgramBuilder::AppendConstantGlobal(uint64_t instr) {
  auto idx = constant_instrs_.size();
  constant_instrs_.push_back(instr);
  return Value(idx, true);
}

uint64_t ProgramBuilder::GetConstantGlobalInstr(Value v) const {
  if (!v.IsConstantGlobal()) {
    throw std::runtime_error(
        "Can't get constant/global from a non constant/global value.");
  }

  return constant_instrs_[v.GetIdx()];
}

FunctionBuilder& ProgramBuilder::GetCurrentFunction() {
  return functions_[current_function_];
}

const FunctionBuilder& ProgramBuilder::GetCurrentFunction() const {
  return functions_[current_function_];
}

BasicBlockRef ProgramBuilder::GenerateBlock() {
  int basic_block_id = GetCurrentFunction().GenerateBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

BasicBlockRef ProgramBuilder::CurrentBlock() {
  int basic_block_id = GetCurrentFunction().GetCurrentBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

bool ProgramBuilder::IsTerminated(BasicBlockRef b) {
  return functions_[b.GetFunctionID()].IsTerminated(b.GetBasicBlockID());
}

void ProgramBuilder::SetCurrentBlock(BasicBlockRef b) {
  // Switching within a function, not allowed unless terminated
  // Switching between functions is fine
  if (current_function_ == b.GetFunctionID() &&
      !GetCurrentFunction().IsTerminated(b.GetBasicBlockID())) {
    throw std::runtime_error(
        "Cannot switch from current block unless it is terminated.");
  }

  current_function_ = b.GetFunctionID();
  GetCurrentFunction().SetCurrentBasicBlock(b.GetBasicBlockID());
}

void ProgramBuilder::Branch(BasicBlockRef b) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::BR))
                                  .SetMarg0(b.GetBasicBlockID())
                                  .Build());
}

void ProgramBuilder::Branch(Value cond, BasicBlockRef b1, BasicBlockRef b2) {
  /*
  if (cond.IsConstantGlobal()) {
    auto value =
        Type1InstructionReader(constant_instrs_[cond.GetIdx()]).Constant();

    if (value == 1) {
      return Branch(b1);
    } else {
      return Branch(b2);
    }
  }
  */

  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::CONDBR))
                                  .SetArg(cond.Serialize())
                                  .SetMarg0(b1.GetBasicBlockID())
                                  .SetMarg1(b2.GetBasicBlockID())
                                  .Build());
}

Value ProgramBuilder::Phi(Type type) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::PHI))
                                         .SetTypeID(type.GetID())
                                         .Build());
}

Value ProgramBuilder::PhiMember(Value v) {
  if (!v.IsConstantGlobal()) {
    auto v_instr = GetCurrentFunction().GetInstruction(v);
    auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

    if (v_opcode == Opcode::GEP) {
      // GEPs are potentially lazily computed so add in a materialize here for
      // the backend so each instruction has at most one lazy operand
      v = GetCurrentFunction().Append(
          Type3InstructionBuilder()
              .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
              .SetArg(v.Serialize())
              .SetTypeID(TypeOf(v).GetID())
              .Build());
    }
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::PHI_MEMBER))
          .SetArg1(v.Serialize())
          .Build());
}

void ProgramBuilder::UpdatePhiMember(Value phi, Value phi_member) {
  GetCurrentFunction().Update(
      phi_member,
      Type2InstructionBuilder(GetCurrentFunction().GetInstruction(phi_member))
          .SetArg0(phi.Serialize())
          .Build());
}

// Memory
Value ProgramBuilder::Alloca(Type t, int num_values) {
  auto type = type_manager_.PointerType(t);
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::ALLOCA))
                                         .SetTypeID(type.GetID())
                                         .SetArg(num_values)
                                         .Build());
}

Value ProgramBuilder::NullPtr(Type t) {
  return AppendConstantGlobal(
      Type3InstructionBuilder()
          .SetOpcode(ConstantOpcodeTo(ConstantOpcode::NULLPTR))
          .SetTypeID(t.GetID())
          .Build());
}

Value ProgramBuilder::PointerCast(Value v, Type t) {
  if (v.IsConstantGlobal()) {
    return AppendConstantGlobal(
        Type3InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::PTR_CAST))
            .SetArg(v.Serialize())
            .SetTypeID(t.GetID())
            .Build());
  }

  auto v_instr = GetCurrentFunction().GetInstruction(v);
  auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

  if (v_opcode == Opcode::GEP) {
    // GEPs are potentially lazily computed so add in a materialize here for
    // the backend so each instruction has at most one lazy operand
    v = GetCurrentFunction().Append(
        Type3InstructionBuilder()
            .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
            .SetArg(v.Serialize())
            .SetTypeID(TypeOf(v).GetID())
            .Build());
  }

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::PTR_CAST))
                                         .SetArg(v.Serialize())
                                         .SetTypeID(t.GetID())
                                         .Build());
}

void ProgramBuilder::StoreI8(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::I8_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

void ProgramBuilder::StoreI16(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::I16_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

void ProgramBuilder::StoreI32(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::I32_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

void ProgramBuilder::StoreI64(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::I64_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

void ProgramBuilder::StoreF64(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::F64_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

void ProgramBuilder::StorePtr(Value ptr, Value v) {
  if (!v.IsConstantGlobal()) {
    auto v_instr = GetCurrentFunction().GetInstruction(v);
    auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

    if (v_opcode == Opcode::GEP) {
      // GEPs are potentially lazily computed so add in a materialize here for
      // the backend so each instruction has at most one lazy operand
      v = GetCurrentFunction().Append(
          Type3InstructionBuilder()
              .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
              .SetArg(v.Serialize())
              .SetTypeID(TypeOf(v).GetID())
              .Build());
    }
  }

  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::PTR_STORE))
                                  .SetArg0(ptr.Serialize())
                                  .SetArg1(v.Serialize())
                                  .Build());
}

Value ProgramBuilder::LoadI8(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I8_LOAD))
                                         .SetArg0(ptr.Serialize())
                                         .Build());
}

Value ProgramBuilder::LoadI16(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I16_LOAD))
                                         .SetArg0(ptr.Serialize())
                                         .Build());
}

Value ProgramBuilder::LoadI32(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I32_LOAD))
                                         .SetArg0(ptr.Serialize())
                                         .Build());
}

Value ProgramBuilder::LoadI64(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_LOAD))
                                         .SetArg0(ptr.Serialize())
                                         .Build());
}

Value ProgramBuilder::LoadF64(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::F64_LOAD))
                                         .SetArg0(ptr.Serialize())
                                         .Build());
}

Value ProgramBuilder::LoadPtr(Value ptr) {
  auto ptr_type = TypeOf(ptr);

  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::PTR_LOAD))
          .SetTypeID(type_manager_.GetPointerElementType(ptr_type).GetID())
          .SetArg(ptr.Serialize())
          .Build());
}

Type ProgramBuilder::OpaqueType(std::string_view name) {
  return type_manager_.OpaqueType(name);
}

Type ProgramBuilder::VoidType() { return type_manager_.VoidType(); }

Type ProgramBuilder::I1Type() { return type_manager_.I1Type(); }

Type ProgramBuilder::I8Type() { return type_manager_.I8Type(); }

Type ProgramBuilder::I16Type() { return type_manager_.I16Type(); }

Type ProgramBuilder::I32Type() { return type_manager_.I32Type(); }

Type ProgramBuilder::I64Type() { return type_manager_.I64Type(); }

Type ProgramBuilder::F64Type() { return type_manager_.F64Type(); }

Type ProgramBuilder::StructType(absl::Span<const Type> types,
                                std::string_view name) {
  if (name.empty()) {
    return type_manager_.StructType(types);
  } else {
    return type_manager_.NamedStructType(types, name);
  }
}

Type ProgramBuilder::GetOpaqueType(std::string_view name) {
  return type_manager_.GetOpaqueType(name);
}

Type ProgramBuilder::GetStructType(std::string_view name) {
  return type_manager_.GetNamedStructType(name);
}

Type ProgramBuilder::PointerType(Type type) {
  return type_manager_.PointerType(type);
}

Type ProgramBuilder::ArrayType(Type type, int len) {
  return type_manager_.ArrayType(type, len);
}

Type ProgramBuilder::FunctionType(Type result, absl::Span<const Type> args) {
  return type_manager_.FunctionType(result, args);
}

Type ProgramBuilder::TypeOf(Value value) const {
  if (value.IsConstantGlobal()) {
    auto instr = GetConstantGlobalInstr(value);
    auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());

    switch (opcode) {
      case ConstantOpcode::I1_CONST:
        return type_manager_.I1Type();

      case ConstantOpcode::I8_CONST:
        return type_manager_.I8Type();

      case ConstantOpcode::I16_CONST:
        return type_manager_.I16Type();

      case ConstantOpcode::I32_CONST:
        return type_manager_.I32Type();

      case ConstantOpcode::I64_CONST:
        return type_manager_.I64Type();

      case ConstantOpcode::F64_CONST:
        return type_manager_.F64Type();

      case ConstantOpcode::PTR_CONST:
        return type_manager_.I8PtrType();

      case ConstantOpcode::PTR_CAST:
      case ConstantOpcode::NULLPTR:
      case ConstantOpcode::FUNC_PTR:
        return static_cast<Type>(Type3InstructionReader(instr).TypeID());

      case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST:
        return type_manager_.I8PtrType();

      case ConstantOpcode::ARRAY_CONST:
        return array_constants_[Type1InstructionReader(instr).Constant()]
            .Type();

      case ConstantOpcode::STRUCT_CONST:
        return struct_constants_[Type1InstructionReader(instr).Constant()]
            .Type();

      case ConstantOpcode::GLOBAL_REF:
        return globals_[Type1InstructionReader(instr).Constant()]
            .PointerToType();
    }
  }

  auto instr = GetCurrentFunction().GetInstruction(value);
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_AND:
    case Opcode::I1_OR:
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I1_LNOT:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE:
    case Opcode::PTR_CMP_NULLPTR:
      return type_manager_.I1Type();

    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_LOAD:
    case Opcode::I1_ZEXT_I8:
      return type_manager_.I8Type();

    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_LOAD:
    case Opcode::I64_TRUNC_I16:
      return type_manager_.I16Type();

    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_LOAD:
    case Opcode::I64_TRUNC_I32:
      return type_manager_.I32Type();

    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_LSHIFT:
    case Opcode::I64_RSHIFT:
    case Opcode::I64_AND:
    case Opcode::I64_XOR:
    case Opcode::I64_OR:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::F64_CONV_I64:
    case Opcode::I64_LOAD:
      return type_manager_.I64Type();

    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64:
    case Opcode::F64_LOAD:
      return type_manager_.F64Type();

    case Opcode::RETURN:
    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE:
    case Opcode::RETURN_VALUE:
    case Opcode::CONDBR:
    case Opcode::BR:
      return type_manager_.VoidType();

    case Opcode::ALLOCA:
    case Opcode::PHI:
    case Opcode::CALL:
    case Opcode::PTR_LOAD:
    case Opcode::PTR_MATERIALIZE:
    case Opcode::PTR_CAST:
    case Opcode::GEP:
    case Opcode::FUNC_ARG:
    case Opcode::CALL_INDIRECT:
      return static_cast<Type>(Type3InstructionReader(instr).TypeID());

    case Opcode::GEP_OFFSET:
      throw std::runtime_error("GEP_OFFSET needs to be under a GEP.");
      break;

    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
      throw std::runtime_error("Cannot call type of on an extension.");
  }
}

FunctionRef ProgramBuilder::CreateFunction(Type result_type,
                                           absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back(*this, "_func" + std::to_string(idx),
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, false, false);

  current_function_ = idx;
  functions_.back().InitBody();

  return static_cast<FunctionRef>(idx);
}

FunctionRef ProgramBuilder::CreatePublicFunction(
    Type result_type, absl::Span<const Type> arg_types, std::string_view name) {
  auto idx = functions_.size();
  functions_.emplace_back(*this, name,
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, false, true);

  current_function_ = idx;
  functions_.back().InitBody();

  auto ref = static_cast<FunctionRef>(idx);
  name_to_function_[name] = ref;
  return ref;
}

FunctionRef ProgramBuilder::DeclareExternalFunction(
    std::string_view name, Type result_type, absl::Span<const Type> arg_types,
    void* func) {
  if (name_to_function_.contains(name)) {
    return name_to_function_[name];
  }

  auto idx = functions_.size();
  functions_.emplace_back(*this, name,
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, true, false, func);
  auto ref = static_cast<FunctionRef>(idx);
  name_to_function_[name] = ref;
  return static_cast<FunctionRef>(idx);
}

FunctionRef ProgramBuilder::GetFunction(std::string_view name) {
  return name_to_function_.at(name);
}

absl::Span<const Value> ProgramBuilder::GetFunctionArguments(FunctionRef func) {
  return functions_[func.GetID()].GetFunctionArguments();
}

Value ProgramBuilder::GetFunctionPointer(FunctionRef func) {
  return AppendConstantGlobal(
      Type3InstructionBuilder()
          .SetOpcode(ConstantOpcodeTo(ConstantOpcode::FUNC_PTR))
          .SetArg(func.GetID())
          .SetTypeID(type_manager_.PointerType(functions_[func.GetID()].Type())
                         .GetID())
          .Build());
}

Value ProgramBuilder::Call(FunctionRef func,
                           absl::Span<const Value> arguments) {
  auto result = functions_[func.GetID()].ReturnType();

  std::vector<Value> materialized_args;
  for (uint8_t i = 0; i < arguments.size(); i++) {
    auto v = arguments[i];
    if (!v.IsConstantGlobal()) {
      auto v_instr = GetCurrentFunction().GetInstruction(v);
      auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

      if (v_opcode == Opcode::GEP) {
        // GEPs are potentially lazily computed so add in a materialize here for
        // the backend so each instruction has at most one lazy operand
        v = GetCurrentFunction().Append(
            Type3InstructionBuilder()
                .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
                .SetArg(v.Serialize())
                .SetTypeID(TypeOf(v).GetID())
                .Build());
      }
    }
    materialized_args.push_back(v);
  }

  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(
        Type3InstructionBuilder()
            .SetOpcode(OpcodeTo(Opcode::CALL_ARG))
            .SetTypeID(TypeOf(materialized_args[i]).GetID())
            .SetSarg(i)
            .SetArg(materialized_args[i].Serialize())
            .Build());
  }

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::CALL))
                                         .SetArg(func.GetID())
                                         .SetTypeID(result.GetID())
                                         .Build());
}

Value ProgramBuilder::Call(Value func, absl::Span<const Value> arguments) {
  std::vector<Value> materialized_args;
  for (uint8_t i = 0; i < arguments.size(); i++) {
    auto v = arguments[i];
    if (!v.IsConstantGlobal()) {
      auto v_instr = GetCurrentFunction().GetInstruction(v);
      auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

      if (v_opcode == Opcode::GEP) {
        // GEPs are potentially lazily computed so add in a materialize here for
        // the backend so each instruction has at most one lazy operand
        v = GetCurrentFunction().Append(
            Type3InstructionBuilder()
                .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
                .SetArg(v.Serialize())
                .SetTypeID(TypeOf(v).GetID())
                .Build());
      }
    }
    materialized_args.push_back(v);
  }

  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(
        Type3InstructionBuilder()
            .SetOpcode(OpcodeTo(Opcode::CALL_ARG))
            .SetTypeID(TypeOf(materialized_args[i]).GetID())
            .SetSarg(i)
            .SetArg(materialized_args[i].Serialize())
            .Build());
  }

  auto func_ptr_type = TypeOf(func);
  if (!type_manager_.IsPtrType(func_ptr_type)) {
    throw std::runtime_error("Must be a func ptr");
  }
  auto func_type = type_manager_.GetPointerElementType(func_ptr_type);
  if (!type_manager_.IsFuncType(func_type)) {
    throw std::runtime_error("Must be a func ptr");
  }
  auto return_type = type_manager_.GetFunctionReturnType(func_type);

  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::CALL_INDIRECT))
          .SetArg(func.Serialize())
          .SetTypeID(return_type.GetID())
          .Build());
}

void ProgramBuilder::Return(Value v) {
  GetCurrentFunction().Append(Type3InstructionBuilder()
                                  .SetOpcode(OpcodeTo(Opcode::RETURN_VALUE))
                                  .SetArg(v.Serialize())
                                  .SetTypeID(TypeOf(v).GetID())
                                  .Build());
}

void ProgramBuilder::Return() {
  GetCurrentFunction().Append(
      Type1InstructionBuilder().SetOpcode(OpcodeTo(Opcode::RETURN)).Build());
}

Value ProgramBuilder::LNotI1(Value v) {
  if (v.IsConstantGlobal()) {
    auto value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    return ConstI1(value == 0 ? true : false);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I1_LNOT))
                                         .SetArg0(v.Serialize())
                                         .Build());
}

Value ProgramBuilder::AndI1(Value v1, Value v2) {
  if (v1.IsConstantGlobal()) {
    auto value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();

    if (value1 == 0) {
      return ConstI1(false);
    } else {
      return v2;
    }
  }

  if (v2.IsConstantGlobal()) {
    auto value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    if (value2 == 0) {
      return ConstI1(false);
    } else {
      return v1;
    }
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I1_AND))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::OrI1(Value v1, Value v2) {
  if (v1.IsConstantGlobal()) {
    auto value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();

    if (value1 == 1) {
      return ConstI1(true);
    } else {
      return v2;
    }
  }

  if (v2.IsConstantGlobal()) {
    auto value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    if (value2 == 1) {
      return ConstI1(true);
    } else {
      return v1;
    }
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I1_OR))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpI1(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    auto value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    auto value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      default:
        throw std::runtime_error("Invalid comp type for I1");
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I1_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I1_CMP_NE;
      break;

    default:
      throw std::runtime_error("Invalid comp type for I1");
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::ConstI1(bool v) {
  if (i1_const_to_value_.find(v) == i1_const_to_value_.end()) {
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::I1_CONST))
            .SetConstant(v ? 1 : 0)
            .Build());
    i1_const_to_value_[v] = value;
  }

  return i1_const_to_value_[v];
}

Value ProgramBuilder::I64ZextI1(Value v) {
  if (v.IsConstantGlobal()) {
    auto value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();

    if (value == 0) {
      return ConstI64(0);
    } else {
      return ConstI64(1);
    }
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I1_ZEXT_I64))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::I8ZextI1(Value v) {
  if (v.IsConstantGlobal()) {
    auto value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();

    if (value == 0) {
      return ConstI8(0);
    } else {
      return ConstI8(1);
    }
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I1_ZEXT_I8))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::AddI8(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int8_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int8_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI8(value1 + value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I8_ADD))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::MulI8(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int8_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int8_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI8(value1 * value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I8_MUL))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::SubI8(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int8_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int8_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI8(value1 - value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I8_SUB))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpI8(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int8_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int8_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      case CompType::LT:
        return ConstI1(value1 < value2);

      case CompType::LE:
        return ConstI1(value1 <= value2);

      case CompType::GT:
        return ConstI1(value1 > value2);

      case CompType::GE:
        return ConstI1(value1 >= value2);
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    switch (cmp) {
      case CompType::EQ:
      case CompType::NE:
        std::swap(v1, v2);
        break;

      case CompType::LT:
        std::swap(v1, v2);
        cmp = CompType::GT;
        break;

      case CompType::LE:
        std::swap(v1, v2);
        cmp = CompType::GE;
        break;

      case CompType::GT:
        std::swap(v1, v2);
        cmp = CompType::LT;
        break;

      case CompType::GE:
        std::swap(v1, v2);
        cmp = CompType::LE;
        break;
    }
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I8_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I8_CMP_NE;
      break;

    case CompType::LT:
      opcode = Opcode::I8_CMP_LT;
      break;

    case CompType::LE:
      opcode = Opcode::I8_CMP_LE;
      break;

    case CompType::GT:
      opcode = Opcode::I8_CMP_GT;
      break;

    case CompType::GE:
      opcode = Opcode::I8_CMP_GE;
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::ConstI8(uint8_t v) {
  if (i8_const_to_value_.find(v) == i8_const_to_value_.end()) {
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::I8_CONST))
            .SetConstant(static_cast<uint8_t>(v))
            .Build());
    i8_const_to_value_[v] = value;
  }

  return i8_const_to_value_[v];
}

Value ProgramBuilder::I64ZextI8(Value v) {
  if (v.IsConstantGlobal()) {
    uint8_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    uint64_t value_as_64 = value;
    return ConstI64(value_as_64);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I8_ZEXT_I64))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::F64ConvI8(Value v) {
  if (v.IsConstantGlobal()) {
    int8_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    double value_as_f = value;
    return ConstF64(value_as_f);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I8_CONV_F64))
          .SetArg0(v.Serialize())
          .Build());
}

// I16
Value ProgramBuilder::AddI16(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int16_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int16_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI16(value1 + value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I16_ADD))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::MulI16(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int16_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int16_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI16(value1 * value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I16_MUL))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::SubI16(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int16_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int16_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI16(value1 - value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I16_SUB))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpI16(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int16_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int16_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      case CompType::LT:
        return ConstI1(value1 < value2);

      case CompType::LE:
        return ConstI1(value1 <= value2);

      case CompType::GT:
        return ConstI1(value1 > value2);

      case CompType::GE:
        return ConstI1(value1 >= value2);
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    switch (cmp) {
      case CompType::EQ:
      case CompType::NE:
        std::swap(v1, v2);
        break;

      case CompType::LT:
        std::swap(v1, v2);
        cmp = CompType::GT;
        break;

      case CompType::LE:
        std::swap(v1, v2);
        cmp = CompType::GE;
        break;

      case CompType::GT:
        std::swap(v1, v2);
        cmp = CompType::LT;
        break;

      case CompType::GE:
        std::swap(v1, v2);
        cmp = CompType::LE;
        break;
    }
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I16_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I16_CMP_NE;
      break;

    case CompType::LT:
      opcode = Opcode::I16_CMP_LT;
      break;

    case CompType::LE:
      opcode = Opcode::I16_CMP_LE;
      break;

    case CompType::GT:
      opcode = Opcode::I16_CMP_GT;
      break;

    case CompType::GE:
      opcode = Opcode::I16_CMP_GE;
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::ConstI16(uint16_t v) {
  if (i16_const_to_value_.find(v) == i16_const_to_value_.end()) {
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::I16_CONST))
            .SetConstant(v)
            .Build());
    i16_const_to_value_[v] = value;
  }

  return i16_const_to_value_[v];
}

Value ProgramBuilder::I64ZextI16(Value v) {
  if (v.IsConstantGlobal()) {
    uint16_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    uint64_t value_zext = value;
    return ConstI64(value_zext);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I16_ZEXT_I64))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::F64ConvI16(Value v) {
  if (v.IsConstantGlobal()) {
    int16_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    double value_as_f = value;
    return ConstF64(value_as_f);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I16_CONV_F64))
          .SetArg0(v.Serialize())
          .Build());
}

// I32
Value ProgramBuilder::AddI32(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int32_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int32_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI32(value1 + value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I32_ADD))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::MulI32(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int32_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int32_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI32(value1 * value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I32_MUL))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::SubI32(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int32_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int32_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    return ConstI32(value1 - value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I32_SUB))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpI32(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int32_t value1 =
        Type1InstructionReader(constant_instrs_[v1.GetIdx()]).Constant();
    int32_t value2 =
        Type1InstructionReader(constant_instrs_[v2.GetIdx()]).Constant();

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      case CompType::LT:
        return ConstI1(value1 < value2);

      case CompType::LE:
        return ConstI1(value1 <= value2);

      case CompType::GT:
        return ConstI1(value1 > value2);

      case CompType::GE:
        return ConstI1(value1 >= value2);
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    switch (cmp) {
      case CompType::EQ:
      case CompType::NE:
        std::swap(v1, v2);
        break;

      case CompType::LT:
        std::swap(v1, v2);
        cmp = CompType::GT;
        break;

      case CompType::LE:
        std::swap(v1, v2);
        cmp = CompType::GE;
        break;

      case CompType::GT:
        std::swap(v1, v2);
        cmp = CompType::LT;
        break;

      case CompType::GE:
        std::swap(v1, v2);
        cmp = CompType::LE;
        break;
    }
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I32_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I32_CMP_NE;
      break;

    case CompType::LT:
      opcode = Opcode::I32_CMP_LT;
      break;

    case CompType::LE:
      opcode = Opcode::I32_CMP_LE;
      break;

    case CompType::GT:
      opcode = Opcode::I32_CMP_GT;
      break;

    case CompType::GE:
      opcode = Opcode::I32_CMP_GE;
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::ConstI32(uint32_t v) {
  if (i32_const_to_value_.find(v) == i32_const_to_value_.end()) {
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::I32_CONST))
            .SetConstant(v)
            .Build());
    i32_const_to_value_[v] = value;
  }

  return i32_const_to_value_[v];
}

Value ProgramBuilder::I64ZextI32(Value v) {
  if (v.IsConstantGlobal()) {
    uint32_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    uint32_t value_zext = value;
    return ConstI64(value_zext);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I32_ZEXT_I64))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::F64ConvI32(Value v) {
  if (v.IsConstantGlobal()) {
    int32_t value =
        Type1InstructionReader(constant_instrs_[v.GetIdx()]).Constant();
    double value_as_f = value;
    return ConstF64(value_as_f);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I32_CONV_F64))
          .SetArg0(v.Serialize())
          .Build());
}

// I64
Value ProgramBuilder::AddI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    int64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 + value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_ADD))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::MulI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    int64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 * value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_MUL))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::SubI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    int64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 - value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_SUB))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::LShiftI64(Value v1, uint8_t v2) {
  if (v1.IsConstantGlobal()) {
    uint64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];

    return ConstI64(value1 << v2);
  }

  auto materialized_v2 = ConstI64(v2);
  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I64_LSHIFT))
          .SetArg0(v1.Serialize())
          .SetArg1(materialized_v2.Serialize())
          .Build());
}

Value ProgramBuilder::RShiftI64(Value v1, uint8_t v2) {
  if (v1.IsConstantGlobal()) {
    uint64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];

    return ConstI64(value1 >> v2);
  }

  auto materialized_v2 = ConstI64(v2);
  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I64_RSHIFT))
          .SetArg0(v1.Serialize())
          .SetArg1(materialized_v2.Serialize())
          .Build());
}

Value ProgramBuilder::I16TruncI64(Value v) {
  if (v.IsConstantGlobal()) {
    uint64_t value =
        i64_constants_[Type1InstructionReader(constant_instrs_[v.GetIdx()])
                           .Constant()];
    uint16_t value_trunc = value;
    return ConstI16(value_trunc);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I64_TRUNC_I16))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::I32TruncI64(Value v) {
  if (v.IsConstantGlobal()) {
    uint64_t value =
        i64_constants_[Type1InstructionReader(constant_instrs_[v.GetIdx()])
                           .Constant()];
    uint32_t value_trunc = value;
    return ConstI32(value_trunc);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I64_TRUNC_I32))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::AndI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    uint64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    uint64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 & value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_AND))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::XorI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    uint64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    uint64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 ^ value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_XOR))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::OrI64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    uint64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    uint64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstI64(value1 | value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::I64_OR))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpI64(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    int64_t value1 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    int64_t value2 =
        i64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      case CompType::LT:
        return ConstI1(value1 < value2);

      case CompType::LE:
        return ConstI1(value1 <= value2);

      case CompType::GT:
        return ConstI1(value1 > value2);

      case CompType::GE:
        return ConstI1(value1 >= value2);
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    switch (cmp) {
      case CompType::EQ:
      case CompType::NE:
        std::swap(v1, v2);
        break;

      case CompType::LT:
        std::swap(v1, v2);
        cmp = CompType::GT;
        break;

      case CompType::LE:
        std::swap(v1, v2);
        cmp = CompType::GE;
        break;

      case CompType::GT:
        std::swap(v1, v2);
        cmp = CompType::LT;
        break;

      case CompType::GE:
        std::swap(v1, v2);
        cmp = CompType::LE;
        break;
    }
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I64_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I64_CMP_NE;
      break;

    case CompType::LT:
      opcode = Opcode::I64_CMP_LT;
      break;

    case CompType::LE:
      opcode = Opcode::I64_CMP_LE;
      break;

    case CompType::GT:
      opcode = Opcode::I64_CMP_GT;
      break;

    case CompType::GE:
      opcode = Opcode::I64_CMP_GE;
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::IsNullPtr(Value v) {
  if (v.IsConstantGlobal()) {
    auto v_opcode = ConstantOpcodeFrom(
        GenericInstructionReader(constant_instrs_[v.GetIdx()]).Opcode());

    if (v_opcode == ConstantOpcode::NULLPTR) {
      return ConstI1(true);
    }
  }

  if (!v.IsConstantGlobal()) {
    auto v_instr = GetCurrentFunction().GetInstruction(v);
    auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

    if (v_opcode == Opcode::GEP) {
      // GEPs are potentially lazily computed so add in a materialize here for
      // the backend so each instruction has at most one lazy operand
      v = GetCurrentFunction().Append(
          Type3InstructionBuilder()
              .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
              .SetArg(v.Serialize())
              .SetTypeID(TypeOf(v).GetID())
              .Build());
    }
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::PTR_CMP_NULLPTR))
          .SetArg0(v.Serialize())
          .Build());
}

Value ProgramBuilder::ConstI64(uint64_t v) {
  if (i64_const_to_value_.find(v) == i64_const_to_value_.end()) {
    uint32_t id = i64_constants_.size();
    i64_constants_.push_back(v);
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::I64_CONST))
            .SetConstant(id)
            .Build());
    i64_const_to_value_[v] = value;
  }

  return i64_const_to_value_[v];
}

Value ProgramBuilder::F64ConvI64(Value v) {
  if (v.IsConstantGlobal()) {
    int64_t value =
        i64_constants_[Type1InstructionReader(constant_instrs_[v.GetIdx()])
                           .Constant()];
    double value_as_f = value;
    return ConstF64(value_as_f);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::I64_CONV_F64))
          .SetArg0(v.Serialize())
          .Build());
}

// F64
Value ProgramBuilder::AddF64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    double value1 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    double value2 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstF64(value1 + value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::F64_ADD))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::MulF64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    double value1 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    double value2 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstF64(value1 * value2);
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    std::swap(v1, v2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::F64_MUL))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::DivF64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    double value1 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    double value2 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstF64(value1 / value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::F64_DIV))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::SubF64(Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    double value1 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    double value2 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    return ConstF64(value1 - value2);
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::F64_SUB))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::CmpF64(CompType cmp, Value v1, Value v2) {
  if (v1.IsConstantGlobal() && v2.IsConstantGlobal()) {
    double value1 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v1.GetIdx()])
                           .Constant()];
    double value2 =
        f64_constants_[Type1InstructionReader(constant_instrs_[v2.GetIdx()])
                           .Constant()];

    switch (cmp) {
      case CompType::EQ:
        return ConstI1(value1 == value2);

      case CompType::NE:
        return ConstI1(value1 != value2);

      case CompType::LT:
        return ConstI1(value1 < value2);

      case CompType::LE:
        return ConstI1(value1 <= value2);

      case CompType::GT:
        return ConstI1(value1 > value2);

      case CompType::GE:
        return ConstI1(value1 >= value2);
    }
  }

  if (v1.IsConstantGlobal() && !v2.IsConstantGlobal()) {
    switch (cmp) {
      case CompType::EQ:
      case CompType::NE:
        std::swap(v1, v2);
        break;

      case CompType::LT:
        std::swap(v1, v2);
        cmp = CompType::GT;
        break;

      case CompType::LE:
        std::swap(v1, v2);
        cmp = CompType::GE;
        break;

      case CompType::GT:
        std::swap(v1, v2);
        cmp = CompType::LT;
        break;

      case CompType::GE:
        std::swap(v1, v2);
        cmp = CompType::LE;
        break;
    }
  }

  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::F64_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::F64_CMP_NE;
      break;

    case CompType::LT:
      opcode = Opcode::F64_CMP_LT;
      break;

    case CompType::LE:
      opcode = Opcode::F64_CMP_LE;
      break;

    case CompType::GT:
      opcode = Opcode::F64_CMP_GT;
      break;

    case CompType::GE:
      opcode = Opcode::F64_CMP_GE;
      break;
  }

  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(OpcodeTo(opcode))
                                         .SetArg0(v1.Serialize())
                                         .SetArg1(v2.Serialize())
                                         .Build());
}

Value ProgramBuilder::ConstF64(double v) {
  if (f64_const_to_value_.find(v) == f64_const_to_value_.end()) {
    uint32_t id = f64_constants_.size();
    f64_constants_.push_back(v);
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::F64_CONST))
            .SetConstant(id)
            .Build());
    f64_const_to_value_[v] = value;
  }

  return f64_const_to_value_[v];
}

Value ProgramBuilder::ConstPtr(void* v) {
  if (ptr_const_to_value_.find(v) == ptr_const_to_value_.end()) {
    uint32_t id = ptr_constants_.size();
    ptr_constants_.push_back(v);
    auto value = AppendConstantGlobal(
        Type1InstructionBuilder()
            .SetOpcode(ConstantOpcodeTo(ConstantOpcode::PTR_CONST))
            .SetConstant(id)
            .Build());
    ptr_const_to_value_[v] = value;
  }

  return ptr_const_to_value_[v];
}

Value ProgramBuilder::I64ConvF64(Value v) {
  if (v.IsConstantGlobal()) {
    double value =
        f64_constants_[Type1InstructionReader(constant_instrs_[v.GetIdx()])
                           .Constant()];
    int64_t value_as_i64 = value;
    return ConstI64(value_as_i64);
  }

  return GetCurrentFunction().Append(
      Type2InstructionBuilder()
          .SetOpcode(OpcodeTo(Opcode::F64_CONV_I64))
          .SetArg0(v.Serialize())
          .Build());
}

// Globals
Value ProgramBuilder::GlobalConstCharArray(std::string_view v) {
  if (string_const_to_value_.find(v) == string_const_to_value_.end()) {
    uint32_t idx = char_array_constants_.size();
    char_array_constants_.emplace_back(v);
    auto value =
        AppendConstantGlobal(Type1InstructionBuilder()
                                 .SetOpcode(ConstantOpcodeTo(
                                     ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST))
                                 .SetConstant(idx)
                                 .Build());
    string_const_to_value_[v] = value;
  }

  return string_const_to_value_[v];
}

Value ProgramBuilder::ConstantStruct(Type t, absl::Span<const Value> init) {
  uint32_t idx = struct_constants_.size();
  struct_constants_.emplace_back(t, init);

  return AppendConstantGlobal(
      Type1InstructionBuilder()
          .SetOpcode(ConstantOpcodeTo(ConstantOpcode::STRUCT_CONST))
          .SetConstant(idx)
          .Build());
}

Value ProgramBuilder::ConstantArray(Type t, absl::Span<const Value> init) {
  uint32_t idx = array_constants_.size();
  array_constants_.emplace_back(t, init);

  return AppendConstantGlobal(
      Type1InstructionBuilder()
          .SetOpcode(ConstantOpcodeTo(ConstantOpcode::ARRAY_CONST))
          .SetConstant(idx)
          .Build());
}

Value ProgramBuilder::Global(Type t, Value init) {
  uint32_t idx = globals_.size();
  auto ptr_to_type = type_manager_.PointerType(t);
  globals_.emplace_back(t, ptr_to_type, init);
  return AppendConstantGlobal(
      Type1InstructionBuilder()
          .SetOpcode(ConstantOpcodeTo(ConstantOpcode::GLOBAL_REF))
          .SetConstant(idx)
          .Build());
}

Value ProgramBuilder::SizeOf(Type type) {
  auto [offset, result_type] = type_manager_.GetPointerOffset(type, {1});
  return ConstI64(offset);
}

Value ProgramBuilder::ConstGEP(Type t, Value v, absl::Span<const int32_t> idx) {
  if (!v.IsConstantGlobal()) {
    auto v_instr = GetCurrentFunction().GetInstruction(v);
    auto v_opcode = OpcodeFrom(GenericInstructionReader(v_instr).Opcode());

    if (v_opcode == Opcode::GEP) {
      // GEPs are potentially lazily computed so add in a materialize here for
      // the backend so each instruction has at most one lazy operand
      v = GetCurrentFunction().Append(
          Type3InstructionBuilder()
              .SetOpcode(OpcodeTo(Opcode::PTR_MATERIALIZE))
              .SetArg(v.Serialize())
              .SetTypeID(TypeOf(v).GetID())
              .Build());
    }
  }

  auto [offset, result_type] = type_manager_.GetPointerOffset(t, idx);
  auto offset_v = ConstI32(offset);

  auto untyped_location_v =
      GetCurrentFunction().Append(Type2InstructionBuilder()
                                      .SetOpcode(OpcodeTo(Opcode::GEP_OFFSET))
                                      .SetArg0(v.Serialize())
                                      .SetArg1(offset_v.Serialize())
                                      .Build());

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(OpcodeTo(Opcode::GEP))
                                         .SetArg(untyped_location_v.Serialize())
                                         .SetTypeID(result_type.GetID())
                                         .Build());
}

const TypeManager& ProgramBuilder::GetTypeManager() const {
  return type_manager_;
}

Program ProgramBuilder::Build() {
  std::vector<Function> functions;
  for (auto& func : functions_) {
    if (func.External()) {
      functions.emplace_back(std::move(func.name_), func.Type(), func.Addr());
      continue;
    }

    std::vector<BasicBlock> basic_blocks;
    for (int i = 0; i < func.basic_blocks_.size(); i++) {
      basic_blocks.emplace_back(
          std::vector<std::pair<int, int>>{func.basic_blocks_[i]},
          func.basic_block_successors_[i], func.basic_block_predecessors_[i]);
    }
    functions.emplace_back(std::move(func.name_), func.Type(), func.Public(),
                           std::move(func.instructions_),
                           std::move(basic_blocks));
  }

  return Program(std::move(type_manager_), std::move(functions),
                 std::move(constant_instrs_), std::move(ptr_constants_),
                 std::move(i64_constants_), std::move(f64_constants_),
                 std::move(char_array_constants_), std::move(struct_constants_),
                 std::move(array_constants_), std::move(globals_));
}

}  // namespace kush::khir