#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/printer.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
class Value {
 public:
  virtual ~Value() = default;

  virtual typename ProgramBuilder<T>::Value& Get() const = 0;

  virtual std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) = 0;
  virtual void Print(proxy::Printer<T>& printer) = 0;
  virtual typename ProgramBuilder<T>::Value& Hash() = 0;
};

}  // namespace kush::compile::proxy