#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class String : public Value<T> {
 public:
  String(ProgramBuilder<T>& program,
         const typename ProgramBuilder<T>::Value& value);

  static String<T> Constant(ProgramBuilder<T>& program, std::string_view value);

  void Copy(const String<T>& rhs);
  void Reset();

  Bool<T> Contains(const String<T>& rhs);
  Bool<T> StartsWith(const String<T>& rhs);
  Bool<T> EndsWith(const String<T>& rhs);
  Bool<T> operator==(const String<T>& rhs);
  Bool<T> operator!=(const String<T>& rhs);
  Bool<T> operator<(const String<T>& rhs);

  std::unique_ptr<String<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;
  void Print(proxy::Printer<T>& printer) override;
  typename ProgramBuilder<T>::Value Hash() override;
  typename ProgramBuilder<T>::Value Get() const override;

  static void ForwardDeclare(ProgramBuilder<T>& program);
  static const std::string_view StringStructName;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value value_;
};

template <typename T>
const std::string_view String<T>::StringStructName("kush::data::String");

}  // namespace kush::compile::proxy