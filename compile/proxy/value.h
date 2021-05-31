#pragma once

#include "compile/proxy/printer.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class Value {
 public:
  virtual ~Value() = default;

  virtual typename khir::Value Get() const = 0;

  virtual std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& rhs) = 0;
  virtual void Print(proxy::Printer& printer) = 0;
  virtual typename khir::Value Hash() = 0;
};

}  // namespace kush::compile::proxy