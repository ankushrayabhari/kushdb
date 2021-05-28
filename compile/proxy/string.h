#pragma once

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class String : public Value {
 public:
  String(khir::KHIRProgramBuilder& program, const khir::Value& value);

  static String Constant(khir::KHIRProgramBuilder& program,
                         std::string_view value);

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

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);
  static const std::string_view StringStructName;

 private:
  khir::KHIRProgramBuilder& program_;
  khir::Value value_;
};

const std::string_view String::StringStructName("kush::data::String");

}  // namespace kush::compile::proxy