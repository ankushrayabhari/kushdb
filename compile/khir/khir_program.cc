#include "compile/khir/khir_program.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/instruction.h"
#include "compile/khir/opcode.h"

namespace kush::khir {

KHIRProgram::Function::Function(Type function_type, Type result_type,
                                absl::Span<const Type> arg_types, bool external)
    : function_type_(function_type), external_(external) {
  if (!external_) {
    for (Type t : arg_types) {
      arg_values_.push_back(Append(Type3InstructionBuilder()
                                       .SetOpcode(Opcode::FUNC_ARG)
                                       .SetTypeID(t.GetID())
                                       .Build()));
    }
  }
}

absl::Span<const Value> KHIRProgram::Function::GetFunctionArguments() const {
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

Value KHIRProgram::Function::Append(uint64_t instr) {
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

void KHIRProgram::Function::Update(Value pos, uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot mutate body of external function");
  }

  auto idx = pos.GetID();
  instructions_[idx] = instr;
}

uint64_t KHIRProgram::Function::GetInstruction(Value v) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return instructions_[v.GetID()];
}

int KHIRProgram::Function::GenerateBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  auto idx = basic_blocks_.size();
  basic_blocks_.push_back({-1, -1});
  return idx;
}

void KHIRProgram::Function::SetCurrentBasicBlock(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  if (!IsTerminated(current_basic_block_)) {
    throw std::runtime_error(
        "Cannot switch from current block unless it is terminated.");
  }

  current_basic_block_ = basic_block_id;
}
int KHIRProgram::Function::GetCurrentBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return current_basic_block_;
}

bool KHIRProgram::Function::IsTerminated(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return basic_blocks_[current_basic_block_].second < 0;
}

KHIRProgram::Function& KHIRProgram::GetCurrentFunction() {
  return functions_[current_function_];
}

BasicBlockRef KHIRProgram::GenerateBlock() {
  int basic_block_id = GetCurrentFunction().GenerateBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

BasicBlockRef KHIRProgram::CurrentBlock() {
  int basic_block_id = GetCurrentFunction().GetCurrentBasicBlock();
  return static_cast<BasicBlockRef>(
      std::pair<int, int>{current_function_, basic_block_id});
}

bool KHIRProgram::IsTerminated(BasicBlockRef b) {
  return functions_[b.GetFunctionID()].IsTerminated(b.GetBasicBlockID());
}

void KHIRProgram::SetCurrentBlock(BasicBlockRef b) {
  current_function_ = b.GetFunctionID();
  GetCurrentFunction().SetCurrentBasicBlock(b.GetBasicBlockID());
}

void KHIRProgram::Branch(BasicBlockRef b) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(Opcode::BR)
                                  .SetMarg0(b.GetBasicBlockID())
                                  .Build());
}

void KHIRProgram::Branch(Value cond, BasicBlockRef b1, BasicBlockRef b2) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(Opcode::CONDBR)
                                  .SetArg(cond.GetID())
                                  .SetMarg0(b1.GetBasicBlockID())
                                  .SetMarg1(b2.GetBasicBlockID())
                                  .Build());
}

Value KHIRProgram::Phi(Type type, uint8_t num_ext) {
  auto v = GetCurrentFunction().Append(Type3InstructionBuilder()
                                           .SetOpcode(Opcode::PHI)
                                           .SetSarg(num_ext)
                                           .SetTypeID(type.GetID())
                                           .Build());

  for (uint8_t i = 0; i < num_ext; i++) {
    GetCurrentFunction().Append(Type5InstructionBuilder()
                                    .SetOpcode(Opcode::PHI_EXT)
                                    .SetMetadata(1)
                                    .Build());
  }

  return v;
}

void KHIRProgram::AddToPhi(Value phi, Value v, BasicBlockRef b) {
  uint32_t offset = 1;
  while (true) {
    Value phi_ext = phi.GetAdjacentInstruction(offset);
    uint64_t instr = GetCurrentFunction().GetInstruction(phi_ext);
    Type5InstructionReader reader(instr);

    if (reader.Opcode() != Opcode::PHI_EXT) {
      break;
    }

    if (reader.Metadata() == 0) {
      offset++;
      continue;
    }

    GetCurrentFunction().Update(phi_ext, Type5InstructionBuilder()
                                             .SetOpcode(Opcode::PHI_EXT)
                                             .SetArg(v.GetID())
                                             .SetMarg0(b.GetBasicBlockID())
                                             .Build());
    return;
  }

  throw std::runtime_error("Attempting to add to phi that is full");
}

// Memory
Value KHIRProgram::Alloca(Type t) {
  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::ALLOCA)
          .SetTypeID(type_manager_.PointerType(t).GetID())
          .Build());
}

Value KHIRProgram::NullPtr(Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::ALLOCA)
                                         .SetTypeID(t.GetID())
                                         .Build());
}

Value KHIRProgram::PointerCast(Value v, Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::PTR_CAST)
                                         .SetArg(v.GetID())
                                         .SetTypeID(t.GetID())
                                         .Build());
}

void KHIRProgram::Store(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

Value KHIRProgram::Load(Value ptr) {
  auto ptr_type = TypeOf(ptr);

  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::LOAD)
          .SetTypeID(type_manager_.GetPointerElementType(ptr_type).GetID())
          .SetArg(ptr.GetID())
          .Build());
}

Type KHIRProgram::VoidType() { return type_manager_.VoidType(); }

Type KHIRProgram::I1Type() { return type_manager_.I1Type(); }

Type KHIRProgram::I8Type() { return type_manager_.I8Type(); }

Type KHIRProgram::I16Type() { return type_manager_.I16Type(); }

Type KHIRProgram::I32Type() { return type_manager_.I32Type(); }

Type KHIRProgram::I64Type() { return type_manager_.I64Type(); }

Type KHIRProgram::F64Type() { return type_manager_.I64Type(); }

Type KHIRProgram::StructType(absl::Span<const Type> types,
                             std::string_view name) {
  if (name.empty()) {
    return type_manager_.StructType(types);
  } else {
    return type_manager_.NamedStructType(types, name);
  }
}

Type KHIRProgram::GetStructType(std::string_view name) {
  return type_manager_.GetNamedStructType(name);
}

Type KHIRProgram::PointerType(Type type) {
  return type_manager_.PointerType(type);
}

Type KHIRProgram::ArrayType(Type type, int len) {
  return type_manager_.ArrayType(type, len);
}

Type KHIRProgram::FunctionType(Type result, absl::Span<const Type> args) {
  return type_manager_.FunctionType(result, args);
}

Type KHIRProgram::TypeOf(Value value) {
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

    case Opcode::GEP:
      throw std::runtime_error("TODO");
      break;

    case Opcode::PHI_EXT:
    case Opcode::CALL_ARG:
      throw std::runtime_error("Cannot call type of on an extension.");
  }
}

FunctionRef KHIRProgram::CreateFunction(Type result_type,
                                        absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back(type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, false);
  current_function_ = idx;
  return static_cast<FunctionRef>(idx);
}

FunctionRef KHIRProgram::CreatePublicFunction(Type result_type,
                                              absl::Span<const Type> arg_types,
                                              std::string_view name) {
  auto ref = CreateFunction(result_type, arg_types);
  name_to_function_[name] = ref;
  return ref;
}

FunctionRef KHIRProgram::DeclareExternalFunction(
    std::string_view name, Type result_type, absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back(type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, true);
  return static_cast<FunctionRef>(idx);
}

FunctionRef KHIRProgram::GetFunction(std::string_view name) {
  return name_to_function_.at(name);
}

absl::Span<const Value> KHIRProgram::GetFunctionArguments(FunctionRef func) {
  return functions_[func.GetID()].GetFunctionArguments();
}

Value KHIRProgram::Call(FunctionRef func, absl::Span<const Value> arguments) {
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

Value KHIRProgram::Call(Value func, Type type,
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

void KHIRProgram::Return(Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::RETURN_VALUE)
                                  .SetArg0(v.GetID())
                                  .Build());
}

void KHIRProgram::Return() {
  GetCurrentFunction().Append(
      Type1InstructionBuilder().SetOpcode(Opcode::RETURN).Build());
}

Value KHIRProgram::LNotI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_LNOT)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value KHIRProgram::CmpI1(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstI1(bool v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I1_CONST)
                                         .SetConstant(v ? 1 : 0)
                                         .Build());
}

Value KHIRProgram::ZextI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value KHIRProgram::AddI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::CmpI8(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstI8(uint8_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I8_CONST)
                                         .SetConstant(static_cast<uint8_t>(v))
                                         .Build());
}

Value KHIRProgram::ZextI8(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I16
Value KHIRProgram::AddI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::CmpI16(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstI16(uint16_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I16_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value KHIRProgram::ZextI16(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I32
Value KHIRProgram::AddI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::CmpI32(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstI32(uint32_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I32_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value KHIRProgram::ZextI32(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I64
Value KHIRProgram::AddI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::CmpI64(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstI64(uint64_t v) {
  uint32_t id = i64_constants.size();
  i64_constants.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value KHIRProgram::F64ConvI64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// F64
Value KHIRProgram::AddF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value KHIRProgram::CmpF64(CompType cmp, Value v1, Value v2) {
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

Value KHIRProgram::ConstF64(double v) {
  uint32_t id = f64_constants.size();
  f64_constants.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value KHIRProgram::I64ConvF64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONV_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

}  // namespace kush::khir