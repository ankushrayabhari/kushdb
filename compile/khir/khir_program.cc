#include "compile/khir/khir_program.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/instruction.h"
#include "compile/khir/opcode.h"

namespace kush::khir {

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

Value KHIRProgram::LNotI1(Value v) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I1_LNOT)
                              .SetArg0(v.GetID())
                              .Build());
  return static_cast<Value>(value);
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

  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(opcode)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::ConstI1(bool v) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type1InstructionBuilder()
                              .SetOpcode(Opcode::I1_CONST)
                              .SetConstant(v ? 1 : 0)
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::AddI8(Value v1, Value v2) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I8_ADD)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::MulI8(Value v1, Value v2) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I8_MUL)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::DivI8(Value v1, Value v2) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I8_DIV)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::SubI8(Value v1, Value v2) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I8_SUB)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
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

  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(opcode)
                              .SetArg0(v1.GetID())
                              .SetArg0(v2.GetID())
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::ConstI8(int8_t v) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type1InstructionBuilder()
                              .SetOpcode(Opcode::I8_CONST)
                              .SetConstant(static_cast<uint8_t>(v))
                              .Build());
  return static_cast<Value>(value);
}

}  // namespace kush::khir