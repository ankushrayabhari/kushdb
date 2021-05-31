#pragma once

#include <memory>

#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class Float64 : public Value {
 public:
  Float64(khir::ProgramBuilder& program, const khir::Value& value);
  Float64(khir::ProgramBuilder& program, double value);
  Float64(khir::ProgramBuilder& program, const proxy::Int8& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int16& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int32& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int64& v);

  khir::Value Get() const override;

  Float64 operator+(const Float64& rhs);
  Float64 operator+(double value);
  Float64 operator-(const Float64& rhs);
  Float64 operator-(double value);
  Float64 operator*(const Float64& rhs);
  Float64 operator*(double value);
  Float64 operator/(const Float64& rhs);
  Float64 operator/(double value);
  Bool operator==(const Float64& rhs);
  Bool operator==(double value);
  Bool operator!=(const Float64& rhs);
  Bool operator!=(double value);
  Bool operator<(const Float64& rhs);
  Bool operator<(double value);
  Bool operator<=(const Float64& rhs);
  Bool operator<=(double value);
  Bool operator>(const Float64& rhs);
  Bool operator>(double value);
  Bool operator>=(const Float64& rhs);
  Bool operator>=(double value);

  std::unique_ptr<Float64> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy