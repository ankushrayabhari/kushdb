#include "compile/proxy/bool.h"

#include <memory>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
Bool<T>::Bool(ProgramBuilder<T>& program,
              typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Bool<T>::Bool(ProgramBuilder<T>& program, bool value)
    : program_(program),
      value_(value ? program_.ConstI8(1) : program_.ConstI8(0)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Bool<T>::Get() const {
  return value_;
}

template <typename T>
std::unique_ptr<Bool<T>> Bool<T>::operator!() {
  return std::make_unique<Bool<T>>(program_, program_.LNotI8(value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Bool<T>::operator==(const Bool<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Bool<T>::operator!=(const Bool<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Value<T>> Bool<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  Bool<T>& rhs_bool = dynamic_cast<Bool<T>&>(rhs);

  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::EQ:
      return this->operator==(rhs_bool);

    case plan::BinaryArithmeticOperatorType::NEQ:
      return this->operator!=(rhs_bool);

    default:
      throw std::runtime_error("Invalid operator on Bool");
  }
}

INSTANTIATE_ON_IR(Bool);

}  // namespace kush::compile::proxy