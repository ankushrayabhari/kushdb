#pragma once

#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T, typename S>
class Ptr : public Value<T> {
 public:
  Ptr(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);

  typename ProgramBuilder<T>::Value& Get() const override;

  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy