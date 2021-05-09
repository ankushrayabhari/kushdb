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
                                absl::Span<const Type> arg_types)
    : function_type_(function_type) {
  for (Type t : arg_types) {
    arg_values_.push_back(Append(Type3InstructionBuilder()
                                     .SetOpcode(Opcode::FUNC_ARG)
                                     .SetTypeID(t.GetID())
                                     .Build()));
  }
}

absl::Span<const Value> KHIRProgram::Function::GetFunctionArguments() const {
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
  auto idx = pos.GetID();
  instructions_[idx] = instr;
}

uint64_t KHIRProgram::Function::GetInstruction(Value v) {
  return instructions_[v.GetID()];
}

int KHIRProgram::Function::GenerateBasicBlock() {
  auto idx = basic_blocks_.size();
  basic_blocks_.push_back({-1, -1});
  return idx;
}

void KHIRProgram::Function::SetCurrentBasicBlock(int basic_block_id) {
  if (!IsTerminated(current_basic_block_)) {
    throw std::runtime_error(
        "Cannot switch from current block unless it is terminated.");
  }

  current_basic_block_ = basic_block_id;
}
int KHIRProgram::Function::GetCurrentBasicBlock() {
  return current_basic_block_;
}

bool KHIRProgram::Function::IsTerminated(int basic_block_id) {
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

FunctionRef KHIRProgram::CreateFunction(Type result_type,
                                        absl::Span<const Type> arg_types) {
  auto idx = functions_.size();
  functions_.emplace_back(type_manager_.FunctionType(result_type, arg_types),
                          result_type, arg_types);
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

FunctionRef KHIRProgram::GetFunction(std::string_view name) {
  return name_to_function_.at(name);
}

absl::Span<const Value> KHIRProgram::GetFunctionArguments(FunctionRef func) {
  return functions_[func.GetID()].GetFunctionArguments();
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
                                         .SetArg0(v2.GetID())
                                         .Build());
}

Value KHIRProgram::ConstI1(bool v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I1_CONST)
                                         .SetConstant(v ? 1 : 0)
                                         .Build());
}

Value KHIRProgram::AddI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_ADD)
                                         .SetArg0(v1.GetID())
                                         .SetArg0(v2.GetID())
                                         .Build());
}

Value KHIRProgram::MulI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_MUL)
                                         .SetArg0(v1.GetID())
                                         .SetArg0(v2.GetID())
                                         .Build());
}

Value KHIRProgram::DivI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_DIV)
                                         .SetArg0(v1.GetID())
                                         .SetArg0(v2.GetID())
                                         .Build());
}

Value KHIRProgram::SubI8(Value v1, Value v2) {
  return GetCurrentFunction().Append(Type2InstructionBuilder()
                                         .SetOpcode(Opcode::I8_SUB)
                                         .SetArg0(v1.GetID())
                                         .SetArg0(v2.GetID())
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
                                         .SetArg0(v2.GetID())
                                         .Build());
}

Value KHIRProgram::ConstI8(int8_t v) {
  return GetCurrentFunction().Append(Type1InstructionBuilder()
                                         .SetOpcode(Opcode::I8_CONST)
                                         .SetConstant(static_cast<uint8_t>(v))
                                         .Build());
}

}  // namespace kush::khir