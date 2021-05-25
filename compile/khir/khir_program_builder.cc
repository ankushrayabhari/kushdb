#include "compile/khir/khir_program_builder.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/instruction.h"
#include "compile/khir/opcode.h"

namespace kush::khir {

KHIRProgramBuilder::Function::Function(std::string_view name,
                                       Type function_type, Type result_type,
                                       absl::Span<const Type> arg_types,
                                       bool external)
    : name_(name), function_type_(function_type), external_(external) {
  if (!external_) {
    for (Type t : arg_types) {
      arg_values_.push_back(Append(Type3InstructionBuilder()
                                       .SetOpcode(Opcode::FUNC_ARG)
                                       .SetTypeID(t.GetID())
                                       .Build()));
    }
  }
}

absl::Span<const Value> KHIRProgramBuilder::Function::GetFunctionArguments()
    const {
  if (external_) {
    throw std::runtime_error(
        "Cannot get argument registers of external function");
  }

  return arg_values_;
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

Value KHIRProgramBuilder::Function::Append(uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot get add body to external function");
  }

  auto idx = instructions_.size();
  instructions_.push_back(instr);

  if (basic_blocks_[current_basic_block_].second > 0) {
    throw std::runtime_error("Cannot append to terminated basic block.");
  }

  if (basic_blocks_[current_basic_block_].first < 0) {
    basic_blocks_[current_basic_block_].first = idx;
  }

  if (basic_blocks_[current_basic_block_].second < 0 &&
      IsTerminatingInstr(GenericInstructionReader(instr).Opcode())) {
    basic_blocks_[current_basic_block_].second = idx;
  }

  return static_cast<Value>(idx);
}

void KHIRProgramBuilder::Function::Update(Value pos, uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot mutate body of external function");
  }

  auto idx = pos.GetID();
  instructions_[idx] = instr;
}

uint64_t KHIRProgramBuilder::Function::GetInstruction(Value v) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return instructions_[v.GetID()];
}

int KHIRProgramBuilder::Function::GenerateBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  auto idx = basic_blocks_.size();
  basic_blocks_.push_back({-1, -1});
  return idx;
}

void KHIRProgramBuilder::Function::SetCurrentBasicBlock(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  if (!IsTerminated(current_basic_block_)) {
    throw std::runtime_error(
        "Cannot switch from current block unless it is terminated.");
  }

  current_basic_block_ = basic_block_id;
}
int KHIRProgramBuilder::Function::GetCurrentBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return current_basic_block_;
}

bool KHIRProgramBuilder::Function::IsTerminated(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return basic_blocks_[current_basic_block_].second < 0;
}

KHIRProgramBuilder::Function& KHIRProgramBuilder::GetCurrentFunction() {
  return functions_[current_function_];
}

BasicBlockRef KHIRProgramBuilder::GenerateBlock() {
  int basic_block_id = GetCurrentFunction().GenerateBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

BasicBlockRef KHIRProgramBuilder::CurrentBlock() {
  int basic_block_id = GetCurrentFunction().GetCurrentBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

bool KHIRProgramBuilder::IsTerminated(BasicBlockRef b) {
  return functions_[b.GetFunctionID()].IsTerminated(b.GetBasicBlockID());
}

void KHIRProgramBuilder::SetCurrentBlock(BasicBlockRef b) {
  current_function_ = b.GetFunctionID();
  GetCurrentFunction().SetCurrentBasicBlock(b.GetBasicBlockID());
}

void KHIRProgramBuilder::Branch(BasicBlockRef b) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(Opcode::BR)
                                  .SetMarg0(b.GetBasicBlockID())
                                  .Build());
}

void KHIRProgramBuilder::Branch(Value cond, BasicBlockRef b1,
                                BasicBlockRef b2) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(Opcode::CONDBR)
                                  .SetArg(cond.GetID())
                                  .SetMarg0(b1.GetBasicBlockID())
                                  .SetMarg1(b2.GetBasicBlockID())
                                  .Build());
}

Value KHIRProgramBuilder::Phi(Type type) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::PHI)
                                         .SetTypeID(type.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::PhiMember(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::PHI_MEMBER)
                                         .SetArg1(v.GetID())
                                         .Build());
}

void KHIRProgramBuilder::UpdatePhiMember(Value phi, Value phi_member) {
  GetCurrentFunction().Update(
      phi_member,
      Type2InstructionBuilder(GetCurrentFunction().GetInstruction(phi_member))
          .SetArg0(phi.GetID())
          .Build());
}

// Memory
Value KHIRProgramBuilder::Alloca(Type t) {
  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::ALLOCA)
          .SetTypeID(type_manager_.PointerType(t).GetID())
          .Build());
}

Value KHIRProgramBuilder::NullPtr(Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::NULLPTR)
                                         .SetTypeID(t.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::PointerCast(Value v, Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::PTR_CAST)
                                         .SetArg(v.GetID())
                                         .SetTypeID(t.GetID())
                                         .Build());
}

void KHIRProgramBuilder::Store(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

Value KHIRProgramBuilder::Load(Value ptr) {
  auto ptr_type = TypeOf(ptr);

  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::LOAD)
          .SetTypeID(type_manager_.GetPointerElementType(ptr_type).GetID())
          .SetArg(ptr.GetID())
          .Build());
}

Type KHIRProgramBuilder::VoidType() { return type_manager_.VoidType(); }

Type KHIRProgramBuilder::I1Type() { return type_manager_.I1Type(); }

Type KHIRProgramBuilder::I8Type() { return type_manager_.I8Type(); }

Type KHIRProgramBuilder::I16Type() { return type_manager_.I16Type(); }

Type KHIRProgramBuilder::I32Type() { return type_manager_.I32Type(); }

Type KHIRProgramBuilder::I64Type() { return type_manager_.I64Type(); }

Type KHIRProgramBuilder::F64Type() { return type_manager_.I64Type(); }

Type KHIRProgramBuilder::StructType(absl::Span<const Type> types,
                                    std::string_view name) {
  if (name.empty()) {
    return type_manager_.StructType(types);
  } else {
    return type_manager_.NamedStructType(types, name);
  }
}

Type KHIRProgramBuilder::GetStructType(std::string_view name) {
  return type_manager_.GetNamedStructType(name);
}

Type KHIRProgramBuilder::PointerType(Type type) {
  return type_manager_.PointerType(type);
}

Type KHIRProgramBuilder::ArrayType(Type type, int len) {
  return type_manager_.ArrayType(type, len);
}

Type KHIRProgramBuilder::FunctionType(Type result,
                                      absl::Span<const Type> args) {
  return type_manager_.FunctionType(result, args);
}

Type KHIRProgramBuilder::TypeOf(Value value) {
  auto instr = GetCurrentFunction().GetInstruction(value);
  auto opcode = GenericInstructionReader(instr).Opcode();

  switch (opcode) {
    case Opcode::I1_CONST:
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
      return type_manager_.I1Type();

    case Opcode::I8_CONST:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_DIV:
      return type_manager_.I8Type();

    case Opcode::I16_CONST:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_DIV:
      return type_manager_.I16Type();

    case Opcode::I32_CONST:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_DIV:
      return type_manager_.I32Type();

    case Opcode::I64_CONST:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_DIV:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::F64_CONV_I64:
      return type_manager_.I64Type();

    case Opcode::F64_CONST:
    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::I64_CONV_F64:
      return type_manager_.F64Type();

    case Opcode::RETURN:
    case Opcode::STORE:
    case Opcode::RETURN_VALUE:
    case Opcode::CONDBR:
    case Opcode::BR:
      return type_manager_.VoidType();

    case Opcode::PHI:
    case Opcode::ALLOCA:
    case Opcode::CALL:
    case Opcode::CALL_INDIRECT:
    case Opcode::LOAD:
    case Opcode::PTR_CAST:
    case Opcode::FUNC_ARG:
    case Opcode::NULLPTR:
      return static_cast<Type>(Type3InstructionReader(instr).TypeID());

    case Opcode::STRING_GLOBAL_CONST:
      return type_manager_.PointerType(type_manager_.I8Type());

    case Opcode::STRUCT_GLOBAL:
      return type_manager_.PointerType(
          global_structs_[Type1InstructionReader(instr).Constant()].Type());

    case Opcode::ARRAY_GLOBAL:
      return type_manager_.PointerType(
          global_arrays_[Type1InstructionReader(instr).Constant()].Type());

    case Opcode::PTR_GLOBAL:
      return type_manager_.PointerType(
          global_pointers_[Type1InstructionReader(instr).Constant()].Type());

    case Opcode::PTR_ADD:
      throw std::runtime_error("PTR_ADD needs to be casted.");
      break;

    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
      throw std::runtime_error("Cannot call type of on an extension.");
  }
}

FunctionRef KHIRProgramBuilder::CreateFunction(
    Type result_type, absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back("_func" + std::to_string(idx),
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, false);
  current_function_ = idx;
  return static_cast<FunctionRef>(idx);
}

FunctionRef KHIRProgramBuilder::CreatePublicFunction(
    Type result_type, absl::Span<const Type> arg_types, std::string_view name) {
  auto ref = CreateFunction(result_type, arg_types);
  name_to_function_[name] = ref;
  return ref;
}

FunctionRef KHIRProgramBuilder::DeclareExternalFunction(
    std::string_view name, Type result_type, absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back(name,
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, true);
  return static_cast<FunctionRef>(idx);
}

FunctionRef KHIRProgramBuilder::GetFunction(std::string_view name) {
  return name_to_function_.at(name);
}

absl::Span<const Value> KHIRProgramBuilder::GetFunctionArguments(
    FunctionRef func) {
  return functions_[func.GetID()].GetFunctionArguments();
}

Value KHIRProgramBuilder::Call(FunctionRef func,
                               absl::Span<const Value> arguments) {
  auto result = functions_[func.GetID()].ReturnType();

  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(Type3InstructionBuilder()
                                    .SetOpcode(Opcode::CALL_ARG)
                                    .SetSarg(i)
                                    .SetArg(arguments[i].GetID())
                                    .Build());
  }

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::CALL)
                                         .SetArg(func.GetID())
                                         .SetTypeID(result.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::Call(Value func, Type type,
                               absl::Span<const Value> arguments) {
  auto result = type_manager_.GetFunctionReturnType(type);

  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(Type3InstructionBuilder()
                                    .SetOpcode(Opcode::CALL_ARG)
                                    .SetSarg(i)
                                    .SetArg(arguments[i].GetID())
                                    .Build());
  }

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::CALL_INDIRECT)
                                         .SetArg(func.GetID())
                                         .SetTypeID(result.GetID())
                                         .Build());
}

void KHIRProgramBuilder::Return(Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::RETURN_VALUE)
                                  .SetArg0(v.GetID())
                                  .Build());
}

void KHIRProgramBuilder::Return() {
  GetCurrentFunction().Append(
      Type1InstructionBuilder().SetOpcode(Opcode::RETURN).Build());
}

Value KHIRProgramBuilder::LNotI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_LNOT)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpI1(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstI1(bool v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I1_CONST)
                                         .SetConstant(v ? 1 : 0)
                                         .Build());
}

Value KHIRProgramBuilder::ZextI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::AddI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::MulI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::DivI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::SubI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpI8(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstI8(uint8_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I8_CONST)
                                         .SetConstant(static_cast<uint8_t>(v))
                                         .Build());
}

Value KHIRProgramBuilder::ZextI8(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I16
Value KHIRProgramBuilder::AddI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::MulI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::DivI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::SubI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpI16(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstI16(uint16_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I16_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value KHIRProgramBuilder::ZextI16(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I32
Value KHIRProgramBuilder::AddI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::MulI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::DivI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::SubI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpI32(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstI32(uint32_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I32_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value KHIRProgramBuilder::ZextI32(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I64
Value KHIRProgramBuilder::AddI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::MulI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::DivI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::SubI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpI64(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstI64(uint64_t v) {
  uint32_t id = i64_constants_.size();
  i64_constants_.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value KHIRProgramBuilder::F64ConvI64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// F64
Value KHIRProgramBuilder::AddF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::MulF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::DivF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::SubF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::CmpF64(CompType cmp, Value v1, Value v2) {
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
                                         .SetOpcode(opcode)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgramBuilder::ConstF64(double v) {
  uint32_t id = f64_constants_.size();
  f64_constants_.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value KHIRProgramBuilder::I64ConvF64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONV_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// Globals
std::function<Value()> KHIRProgramBuilder::GlobalConstCharArray(
    std::string_view s) {
  uint32_t idx = global_char_arrays_.size();
  global_char_arrays_.emplace_back(s);
  return [idx, this]() {
    return this->GetCurrentFunction().Append(
        Type1InstructionBuilder()
            .SetOpcode(Opcode::STRING_GLOBAL_CONST)
            .SetConstant(idx)
            .Build());
  };
}

std::function<Value()> KHIRProgramBuilder::GlobalStruct(
    bool constant, Type t, absl::Span<const Value> init) {
  std::vector<uint64_t> instrs;
  for (const Value v : init) {
    instrs.push_back(GetCurrentFunction().GetInstruction(v));
  }

  uint32_t idx = global_structs_.size();
  global_structs_.emplace_back(constant, t, instrs);
  return [idx, this]() {
    return this->GetCurrentFunction().Append(
        Type1InstructionBuilder()
            .SetOpcode(Opcode::STRUCT_GLOBAL)
            .SetConstant(idx)
            .Build());
  };
}

std::function<Value()> KHIRProgramBuilder::GlobalArray(
    bool constant, Type t, absl::Span<const Value> init) {
  std::vector<uint64_t> instrs;
  for (const Value v : init) {
    instrs.push_back(GetCurrentFunction().GetInstruction(v));
  }

  uint32_t idx = global_arrays_.size();
  global_arrays_.emplace_back(constant, t, instrs);
  return [idx, this]() {
    return this->GetCurrentFunction().Append(
        Type1InstructionBuilder()
            .SetOpcode(Opcode::ARRAY_GLOBAL)
            .SetConstant(idx)
            .Build());
  };
}

std::function<Value()> KHIRProgramBuilder::GlobalPointer(bool constant, Type t,
                                                         Value init) {
  uint32_t idx = global_pointers_.size();
  global_pointers_.emplace_back(constant, t,
                                GetCurrentFunction().GetInstruction(init));
  return [idx, this]() {
    return this->GetCurrentFunction().Append(Type1InstructionBuilder()
                                                 .SetOpcode(Opcode::PTR_GLOBAL)
                                                 .SetConstant(idx)
                                                 .Build());
  };
}

Value KHIRProgramBuilder::SizeOf(Type type) {
  auto pointer_type = PointerType(type);
  auto null = NullPtr(pointer_type);
  auto size_ptr = GetElementPtr(type, null, {1});
  return PointerCast(size_ptr, I64Type());
}

Value KHIRProgramBuilder::GetElementPtr(Type t, Value ptr,
                                        absl::Span<const int32_t> idx) {
  auto [offset, result_type] = type_manager_.GetPointerOffset(t, idx);
  auto offset_v = ConstI64(offset);

  auto untyped_location_v =
      GetCurrentFunction().Append(Type2InstructionBuilder()
                                      .SetOpcode(Opcode::PTR_ADD)
                                      .SetArg0(ptr.GetID())
                                      .SetArg1(offset_v.GetID())
                                      .Build());

  return PointerCast(untyped_location_v, result_type);
}

}  // namespace kush::khir