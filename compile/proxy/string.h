#pragma once

#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class String : public Value {
 public:
  String(khir::ProgramBuilder& program, const khir::Value& value);

  static String Global(khir::ProgramBuilder& program, std::string_view value);
  static khir::Value Constant(khir::ProgramBuilder& program, std::string_view value);

  void Copy(const String& rhs);
  void Reset();

  Bool Contains(const String& rhs);
  Bool StartsWith(const String& rhs);
  Bool EndsWith(const String& rhs);
  Bool operator==(const String& rhs);
  Bool operator!=(const String& rhs);
  Bool operator<(const String& rhs);

  std::unique_ptr<String> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;
  khir::Value Get() const override;

  static void ForwardDeclare(khir::ProgramBuilder& program);
  static const std::string_view StringStructName;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy