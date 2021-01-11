#pragma once

#include <string>
#include <string_view>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"
#include "compile/cpp/proxy/value.h"

namespace kush::compile::cpp::proxy {

class Double : public Value {
 public:
  explicit Double(CppProgram& program);
  Double(CppProgram& program, double value);

  std::string_view Get() override;

  void operator=(Double& rhs);
  std::unique_ptr<Double> operator+(Double& rhs);
  std::unique_ptr<Double> operator-(Double& rhs);
  std::unique_ptr<Double> operator*(Double& rhs);
  std::unique_ptr<Double> operator/(Double& rhs);
  std::unique_ptr<Boolean> operator==(Double& rhs);
  std::unique_ptr<Boolean> operator!=(Double& rhs);
  std::unique_ptr<Boolean> operator<(Double& rhs);
  std::unique_ptr<Boolean> operator<=(Double& rhs);
  std::unique_ptr<Boolean> operator>(Double& rhs);
  std::unique_ptr<Boolean> operator>=(Double& rhs);

 private:
  CppProgram& program_;
  std::string value_;
};

}  // namespace kush::compile::cpp::proxy