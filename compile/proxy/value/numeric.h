#pragma once

#include <memory>

#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class IRValue;
class Float64;

template <typename V>
std::unique_ptr<IRValue> EvaluateBinaryNumeric(
    plan::BinaryArithmeticOperatorType op_type, const V& lhs,
    const IRValue& rhs_generic) {
  const V& rhs = dynamic_cast<const V&>(rhs_generic);

  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::ADD:
      return (lhs + rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::SUB:
      return (lhs - rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::MUL:
      return (lhs * rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::DIV:
      if constexpr (std::is_same_v<V, Float64>) {
        return (lhs / rhs).ToPointer();
      } else {
        throw std::runtime_error("Invalid operator on numeric");
      }

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
