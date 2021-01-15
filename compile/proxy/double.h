#pragma once

#include <string>
#include <string_view>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class Double : public Value {
 public:
  explicit Double(CppProgram& program);
  Double(CppProgram& program, double value);

  std::string_view Get() const override;

  void Assign(Double& rhs);
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

}  // namespace kush::compile::proxy