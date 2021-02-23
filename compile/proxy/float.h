#pragma once

#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
class Float64 : public Value<T> {
 public:
  Float64(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Float64(ProgramBuilder<T>& program, double value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Float64<T> operator+(const Float64<T>& rhs);
  Float64<T> operator-(const Float64<T>& rhs);
  Float64<T> operator*(const Float64<T>& rhs);
  Float64<T> operator/(const Float64<T>& rhs);
  Bool<T> operator==(const Float64<T>& rhs);
  Bool<T> operator!=(const Float64<T>& rhs);
  Bool<T> operator<(const Float64<T>& rhs);
  Bool<T> operator<=(const Float64<T>& rhs);
  Bool<T> operator>(const Float64<T>& rhs);
  Bool<T> operator>=(const Float64<T>& rhs);

  std::unique_ptr<Float64<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;
  void Print(proxy::Printer<T>& printer) override;
  typename ProgramBuilder<T>::Value& Hash() override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy