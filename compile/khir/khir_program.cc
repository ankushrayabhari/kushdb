#include "compile/khir/khir_program.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/instruction.h"
#include "compile/khir/opcode.h"

namespace kush::khir {

class Function {
 public:
 private:
  Type return_type_;
  std::vector<Type> arg_types_;
  Type function_type_;
  std::vector<uint64_t> instructions_;
};

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

Value KHIRProgram::Function::Append(uint64_t instr) {
  auto idx = instructions_.size();
  instructions_.push_back(instr);
  return static_cast<Value>(idx);
}

KHIRProgram::Function& KHIRProgram::GetCurrentFunction() {
  return functions_[current_function_];
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