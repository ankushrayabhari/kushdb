#pragma once

#include <string>
#include <string_view>

#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::cpp::proxy {

class Value {
 public:
  virtual ~Value() = default;

  virtual std::string_view Get() const = 0;
  std::unique_ptr<Value> Ref();

  virtual std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op, const Value& rhs) {
    return nullptr;
  };
};

}  // namespace kush::compile::cpp::proxy