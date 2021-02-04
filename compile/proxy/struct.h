#pragma once

#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
class Struct : public Value<T> {
 public:
  Struct(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);

  typename ProgramBuilder<T>::Value& Get() const override;

  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override {
    throw std::runtime_error("No binary operators on struct");
  }

  void Store(S& data) { program_.Store(value_, data.Get()); }

  S Load() { program_.Load(value_) }

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy