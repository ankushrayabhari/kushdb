#include <memory>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

Bool::Bool(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Bool::Bool(khir::ProgramBuilder& program, bool value)
    : program_(program), value_(program_.ConstI1(value)) {}

khir::Value Bool::Get() const { return value_; }

Bool Bool::operator!() const { return Bool(program_, program_.LNotI1(value_)); }

Bool Bool::operator==(const Bool& rhs) const {
  return Bool(program_, program_.CmpI1(khir::CompType::EQ, value_, rhs.value_));
}

Bool Bool::operator!=(const Bool& rhs) const {
  return Bool(program_, program_.CmpI1(khir::CompType::NE, value_, rhs.value_));
}

std::unique_ptr<Bool> Bool::ToPointer() const {
  return std::make_unique<Bool>(program_, value_);
}

std::unique_ptr<IRValue> Bool::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, const IRValue& rhs) const {
  const Bool& rhs_bool = dynamic_cast<const Bool&>(rhs);

  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::EQ:
      return (*this == rhs_bool).ToPointer();

    case plan::BinaryArithmeticOperatorType::NEQ:
      return (*this != rhs_bool).ToPointer();

    default:
      throw std::runtime_error("Invalid operator on Bool");
  }
}

void Bool::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Bool::Hash() const {
  return proxy::Int64(program_, program_.I64ZextI1(value_));
}

}  // namespace kush::compile::proxy