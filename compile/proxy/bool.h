#pragma once

#include <memory>

#include "compile/proxy/printer.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class Bool : public Value {
 public:
  Bool(khir::ProgramBuilder& program, const khir::Value& value);
  Bool(khir::ProgramBuilder& program, bool value);

  khir::Value Get() const override;

  Bool operator!();
  Bool operator==(const Bool& rhs);
  Bool operator!=(const Bool& rhs);

  std::unique_ptr<Bool> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& rhs) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy