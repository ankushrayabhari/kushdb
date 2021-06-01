#include "khir/program_builder.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

Function::Function(std::string_view name, khir::Type function_type,
                   khir::Type result_type,
                   absl::Span<const khir::Type> arg_types, bool external,
                   bool p, void* func)
    : name_(name),
      return_type_(result_type),
      arg_types_(arg_types.begin(), arg_types.end()),
      function_type_(function_type),
      func_(func),
      external_(external),
      public_(p) {}

void Function::InitBody() {
  GenerateBasicBlock();
  for (int i = 0; i < arg_types_.size(); i++) {
    arg_values_.push_back(Append(Type3InstructionBuilder()
                                     .SetOpcode(Opcode::FUNC_ARG)
                                     .SetSarg(i)
                                     .SetTypeID(arg_types_[i].GetID())
                                     .Build()));
  }
}

absl::Span<const Value> Function::GetFunctionArguments() const {
  if (external_) {
    throw std::runtime_error(
        "Cannot get argument registers of external function");
  }

  return arg_values_;
}

khir::Type Function::ReturnType() const { return return_type_; }

khir::Type Function::Type() const { return function_type_; }

bool Function::External() const { return external_; }

bool Function::Public() const { return public_; }

void* Function::Addr() const { return func_; }

std::string_view Function::Name() const { return name_; }

const std::vector<int>& Function::BasicBlockOrder() const {
  return basic_block_order_;
}

const std::vector<std::pair<int, int>>& Function::BasicBlocks() const {
  return basic_blocks_;
}

const std::vector<uint64_t>& Function::Instructions() const {
  return instructions_;
}

StructConstant::StructConstant(khir::Type type,
                               absl::Span<const uint64_t> fields)
    : type_(type), fields_(fields.begin(), fields.end()) {}

khir::Type StructConstant::Type() const { return type_; }

absl::Span<const uint64_t> StructConstant::Fields() const { return fields_; }

ArrayConstant::ArrayConstant(khir::Type type,
                             absl::Span<const uint64_t> elements)
    : type_(type), elements_(elements.begin(), elements.end()) {}

khir::Type ArrayConstant::Type() const { return type_; }

absl::Span<const uint64_t> ArrayConstant::Elements() const { return elements_; }

Global::Global(bool constant, bool pub, khir::Type type, uint64_t init)
    : constant_(constant), public_(pub), type_(type), init_(init) {}

bool Global::Constant() const { return constant_; }

bool Global::Public() const { return public_; }

Type Global::Type() const { return type_; }

uint64_t Global::InitialValue() const { return init_; }

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

Value Function::Append(uint64_t instr) {
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
      IsTerminatingInstr(GenericInstructionReader(instr).Opcode())) {
    basic_blocks_[current_basic_block_].second = idx;
    basic_block_order_.push_back(current_basic_block_);
  }

  return static_cast<Value>(idx);
}

void Function::Update(Value pos, uint64_t instr) {
  if (external_) {
    throw std::runtime_error("Cannot mutate body of external function");
  }

  auto idx = pos.GetID();
  instructions_[idx] = instr;
}

uint64_t Function::GetInstruction(Value v) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return instructions_[v.GetID()];
}

int Function::GenerateBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  auto idx = basic_blocks_.size();
  basic_blocks_.push_back({-1, -1});
  return idx;
}

void Function::SetCurrentBasicBlock(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot update body of external function");
  }

  current_basic_block_ = basic_block_id;
}
int Function::GetCurrentBasicBlock() {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return current_basic_block_;
}

bool Function::IsTerminated(int basic_block_id) {
  if (external_) {
    throw std::runtime_error("Cannot get body of external function");
  }

  return basic_blocks_[current_basic_block_].second >= 0;
}

Function& ProgramBuilder::GetCurrentFunction() {
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
                                  .SetOpcode(Opcode::BR)
                                  .SetMarg0(b.GetBasicBlockID())
                                  .Build());
}

void ProgramBuilder::Branch(Value cond, BasicBlockRef b1, BasicBlockRef b2) {
  GetCurrentFunction().Append(Type5InstructionBuilder()
                                  .SetOpcode(Opcode::CONDBR)
                                  .SetArg(cond.GetID())
                                  .SetMarg0(b1.GetBasicBlockID())
                                  .SetMarg1(b2.GetBasicBlockID())
                                  .Build());
}

Value ProgramBuilder::Phi(Type type) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::PHI)
                                         .SetTypeID(type.GetID())
                                         .Build());
}

Value ProgramBuilder::PhiMember(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::PHI_MEMBER)
                                         .SetArg1(v.GetID())
                                         .Build());
}

void ProgramBuilder::UpdatePhiMember(Value phi, Value phi_member) {
  GetCurrentFunction().Update(
      phi_member,
      Type2InstructionBuilder(GetCurrentFunction().GetInstruction(phi_member))
          .SetArg0(phi.GetID())
          .Build());
}

// Memory
Value ProgramBuilder::Alloca(Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::ALLOCA)
                                         .SetTypeID(t.GetID())
                                         .Build());
}

Value ProgramBuilder::NullPtr(Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::NULLPTR)
                                         .SetTypeID(t.GetID())
                                         .Build());
}

Value ProgramBuilder::PointerCast(Value v, Type t) {
  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::PTR_CAST)
                                         .SetArg(v.GetID())
                                         .SetTypeID(t.GetID())
                                         .Build());
}

void ProgramBuilder::StoreI8(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::I8_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

void ProgramBuilder::StoreI16(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::I16_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

void ProgramBuilder::StoreI32(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::I32_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

void ProgramBuilder::StoreI64(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::I64_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

void ProgramBuilder::StoreF64(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::F64_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

void ProgramBuilder::StorePtr(Value ptr, Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::PTR_STORE)
                                  .SetArg0(ptr.GetID())
                                  .SetArg1(v.GetID())
                                  .Build());
}

Value ProgramBuilder::LoadI8(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_LOAD)
                                         .SetArg0(ptr.GetID())
                                         .Build());
}

Value ProgramBuilder::LoadI16(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_LOAD)
                                         .SetArg0(ptr.GetID())
                                         .Build());
}

Value ProgramBuilder::LoadI32(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_LOAD)
                                         .SetArg0(ptr.GetID())
                                         .Build());
}

Value ProgramBuilder::LoadI64(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_LOAD)
                                         .SetArg0(ptr.GetID())
                                         .Build());
}

Value ProgramBuilder::LoadF64(Value ptr) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_LOAD)
                                         .SetArg0(ptr.GetID())
                                         .Build());
}

Value ProgramBuilder::LoadPtr(Value ptr) {
  auto ptr_type = TypeOf(ptr);

  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::PTR_LOAD)
          .SetTypeID(type_manager_.GetPointerElementType(ptr_type).GetID())
          .SetArg(ptr.GetID())
          .Build());
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

Type ProgramBuilder::TypeOf(Value value) {
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
    case Opcode::I8_LOAD:
    case Opcode::I1_ZEXT_I8:
      return type_manager_.I8Type();

    case Opcode::I16_CONST:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_DIV:
    case Opcode::I16_LOAD:
      return type_manager_.I16Type();

    case Opcode::I32_CONST:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_DIV:
    case Opcode::I32_LOAD:
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
    case Opcode::I64_LOAD:
      return type_manager_.I64Type();

    case Opcode::F64_CONST:
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

    case Opcode::CALL_INDIRECT:
      return type_manager_.GetFunctionReturnType(
          static_cast<Type>(Type3InstructionReader(instr).TypeID()));

    case Opcode::ALLOCA:
      return type_manager_.PointerType(
          static_cast<Type>(Type3InstructionReader(instr).TypeID()));

    case Opcode::PHI:
    case Opcode::CALL:
    case Opcode::PTR_LOAD:
    case Opcode::PTR_CAST:
    case Opcode::FUNC_ARG:
    case Opcode::NULLPTR:
    case Opcode::FUNC_PTR:
      return static_cast<Type>(Type3InstructionReader(instr).TypeID());

    case Opcode::GLOBAL_CHAR_ARRAY_CONST:
      return type_manager_.PointerType(type_manager_.I8Type());

    case Opcode::STRUCT_CONST:
      return struct_constants_[Type1InstructionReader(instr).Constant()].Type();

    case Opcode::ARRAY_CONST:
      return array_constants_[Type1InstructionReader(instr).Constant()].Type();

    case Opcode::GLOBAL_REF:
      return type_manager_.PointerType(
          globals_[Type1InstructionReader(instr).Constant()].Type());

    case Opcode::PTR_ADD:
      throw std::runtime_error("PTR_ADD needs to be casted.");
      break;

    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
      throw std::runtime_error("Cannot call type of on an extension.");
  }
}

FunctionRef ProgramBuilder::CreateFunction(Type result_type,
                                           absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back("_func" + std::to_string(idx),
                          type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types, false, false);

  current_function_ = idx;
  functions_.back().InitBody();

  return static_cast<FunctionRef>(idx);
}

FunctionRef ProgramBuilder::CreatePublicFunction(
    Type result_type, absl::Span<const Type> arg_types, std::string_view name) {
  auto idx = functions_.size();
  functions_.emplace_back(name,
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
  functions_.emplace_back(name,
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
  return GetCurrentFunction().Append(
      Type3InstructionBuilder()
          .SetOpcode(Opcode::FUNC_PTR)
          .SetArg(func.GetID())
          .SetTypeID(type_manager_.PointerType(functions_[func.GetID()].Type())
                         .GetID())
          .Build());
}

Value ProgramBuilder::Call(FunctionRef func,
                           absl::Span<const Value> arguments) {
  auto result = functions_[func.GetID()].ReturnType();

  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(Type3InstructionBuilder()
                                    .SetOpcode(Opcode::CALL_ARG)
                                    .SetTypeID(TypeOf(arguments[i]).GetID())
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

Value ProgramBuilder::Call(Value func, Type type,
                           absl::Span<const Value> arguments) {
  for (uint8_t i = 0; i < arguments.size(); i++) {
    GetCurrentFunction().Append(Type3InstructionBuilder()
                                    .SetOpcode(Opcode::CALL_ARG)
                                    .SetTypeID(TypeOf(arguments[i]).GetID())
                                    .SetSarg(i)
                                    .SetArg(arguments[i].GetID())
                                    .Build());
  }

  return GetCurrentFunction().Append(Type3InstructionBuilder()
                                         .SetOpcode(Opcode::CALL_INDIRECT)
                                         .SetArg(func.GetID())
                                         .SetTypeID(type.GetID())
                                         .Build());
}

void ProgramBuilder::Return(Value v) {
  GetCurrentFunction().Append(Type2InstructionBuilder()
                                  .SetOpcode(Opcode::RETURN_VALUE)
                                  .SetArg0(v.GetID())
                                  .Build());
}

void ProgramBuilder::Return() {
  GetCurrentFunction().Append(
      Type1InstructionBuilder().SetOpcode(Opcode::RETURN).Build());
}

Value ProgramBuilder::LNotI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_LNOT)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpI1(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstI1(bool v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I1_CONST)
                                         .SetConstant(v ? 1 : 0)
                                         .Build());
}

Value ProgramBuilder::I64ZextI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::I8ZextI1(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I1_ZEXT_I8)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::AddI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::MulI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::DivI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::SubI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpI8(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstI8(uint8_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I8_CONST)
                                         .SetConstant(static_cast<uint8_t>(v))
                                         .Build());
}

Value ProgramBuilder::I64ZextI8(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::F64ConvI8(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I16
Value ProgramBuilder::AddI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::MulI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::DivI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::SubI16(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpI16(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstI16(uint16_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I16_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value ProgramBuilder::I64ZextI16(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::F64ConvI16(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I16_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I32
Value ProgramBuilder::AddI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::MulI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::DivI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::SubI32(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpI32(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstI32(uint32_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I32_CONST)
                                         .SetConstant(v)
                                         .Build());
}

Value ProgramBuilder::I64ZextI32(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_ZEXT_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

Value ProgramBuilder::F64ConvI32(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I32_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// I64
Value ProgramBuilder::AddI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::MulI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::DivI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::SubI64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpI64(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstI64(uint64_t v) {
  uint32_t id = i64_constants_.size();
  i64_constants_.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value ProgramBuilder::F64ConvI64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I64_CONV_F64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// F64
Value ProgramBuilder::AddF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::MulF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::DivF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::SubF64(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg1(v2.GetID())
                                         .Build());
}

Value ProgramBuilder::CmpF64(CompType cmp, Value v1, Value v2) {
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

Value ProgramBuilder::ConstF64(double v) {
  uint32_t id = f64_constants_.size();
  f64_constants_.push_back(v);
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONST)
                                         .SetConstant(id)
                                         .Build());
}

Value ProgramBuilder::I64ConvF64(Value v) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::F64_CONV_I64)
                                         .SetArg0(v.GetID())
                                         .Build());
}

// Globals
std::function<Value()> ProgramBuilder::GlobalConstCharArray(
    std::string_view s) {
  uint32_t idx = char_array_constants_.size();
  char_array_constants_.emplace_back(s);
  return [idx, this]() {
    return this->GetCurrentFunction().Append(
        Type1InstructionBuilder()
            .SetOpcode(Opcode::GLOBAL_CHAR_ARRAY_CONST)
            .SetConstant(idx)
            .Build());
  };
}

std::function<Value()> ProgramBuilder::ConstantStruct(
    Type t, absl::Span<const Value> init) {
  uint32_t idx = struct_constants_.size();

  std::vector<uint64_t> init_instrs;
  for (auto v : init) {
    init_instrs.push_back(GetCurrentFunction().GetInstruction(v));
  }
  struct_constants_.emplace_back(t, init_instrs);

  return [idx, this]() {
    return this->GetCurrentFunction().Append(
        Type1InstructionBuilder()
            .SetOpcode(Opcode::STRUCT_CONST)
            .SetConstant(idx)
            .Build());
  };
}

std::function<Value()> ProgramBuilder::ConstantArray(
    Type t, absl::Span<const Value> init) {
  uint32_t idx = array_constants_.size();

  std::vector<uint64_t> init_instrs;
  for (auto v : init) {
    init_instrs.push_back(GetCurrentFunction().GetInstruction(v));
  }
  array_constants_.emplace_back(t, init_instrs);

  return [idx, this]() {
    return this->GetCurrentFunction().Append(Type1InstructionBuilder()
                                                 .SetOpcode(Opcode::ARRAY_CONST)
                                                 .SetConstant(idx)
                                                 .Build());
  };
}

std::function<Value()> ProgramBuilder::Global(bool constant, bool pub, Type t,
                                              Value init) {
  uint32_t idx = globals_.size();
  globals_.emplace_back(constant, pub, t,
                        GetCurrentFunction().GetInstruction(init));
  return [idx, this]() {
    return this->GetCurrentFunction().Append(Type1InstructionBuilder()
                                                 .SetOpcode(Opcode::GLOBAL_REF)
                                                 .SetConstant(idx)
                                                 .Build());
  };
}

Value ProgramBuilder::SizeOf(Type type) {
  auto pointer_type = PointerType(type);
  auto null = NullPtr(pointer_type);
  auto size_ptr = GetElementPtr(type, null, {1});
  return PointerCast(size_ptr, I64Type());
}

Value ProgramBuilder::GetElementPtr(Type t, Value ptr,
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

void ProgramBuilder::Translate(Backend& backend) {
  backend.Translate(type_manager_, i64_constants_, f64_constants_,
                    char_array_constants_, struct_constants_, array_constants_,
                    globals_, functions_);
}

}  // namespace kush::khir