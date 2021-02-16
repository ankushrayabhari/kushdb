#pragma once

#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
class Bool : public Value<T> {
 public:
  Bool(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Bool(ProgramBuilder<T>& program, bool value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Bool<T> operator!();
  Bool<T> operator==(const Bool<T>& rhs);
  Bool<T> operator!=(const Bool<T>& rhs);

  std::unique_ptr<Bool<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;
  void Print(proxy::Printer<T>& printer) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy