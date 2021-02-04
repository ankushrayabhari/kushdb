#pragma once

#include <memory>

#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename V, typename T>
std::unique_ptr<Value<T>> EvaluateBinaryNumeric(
    plan::BinaryArithmeticOperatorType op_type, V& lhs, Value<T>& rhs_generic) {
  V& rhs = dynamic_cast<V&>(rhs_generic);

  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::ADD:
      return (lhs + rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::SUB:
      return (lhs - rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::MUL:
      return (lhs * rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::DIV:
      return (lhs / rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::EQ:
      return (lhs == rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::NEQ:
      return (lhs != rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::LT:
      return (lhs < rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::LEQ:
      return (lhs <= rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::GT:
      return (lhs > rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::GEQ:
      return (lhs >= rhs).ToPointer();

    default:
      throw std::runtime_error("Invalid operator on numeric");
  }
}

}  // namespace kush::compile::proxy
