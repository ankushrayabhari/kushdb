#include "compile/proxy/bool.h"

#include <memory>

#include "compile/ir_registry.h"
#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

Bool::Bool(khir::KHIRProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Bool::Bool(khir::KHIRProgramBuilder& program, bool value)
    : program_(program), value_(program_.ConstI1(value)) {}

khir::Value Bool::Get() const { return value_; }

Bool Bool::operator!() { return Bool(program_, program_.LNotI1(value_)); }

Bool Bool::operator==(const Bool& rhs) {
  return Bool(program_, program_.CmpI1(khir::CompType::EQ, value_, rhs.value_));
}

Bool Bool::operator!=(const Bool& rhs) {
  return Bool(program_, program_.CmpI1(khir::CompType::NE, value_, rhs.value_));
}

std::unique_ptr<Bool> Bool::ToPointer() {
  return std::make_unique<Bool>(program_, value_);
}

std::unique_ptr<Value> Bool::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value& rhs) {
  Bool& rhs_bool = dynamic_cast<Bool&>(rhs);

  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::EQ:
      return (*this == rhs_bool).ToPointer();

    case plan::BinaryArithmeticOperatorType::NEQ:
      return (*this != rhs_bool).ToPointer();

    default:
      throw std::runtime_error("Invalid operator on Bool");
  }
}

void Bool::Print(proxy::Printer& printer) { printer.Print(*this); }

khir::Value Bool::Hash() { return program_.ZextI1(value_); }

}  // namespace kush::compile::proxy