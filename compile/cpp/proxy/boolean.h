#pragma once

#include <string>
#include <string_view>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/value.h"

namespace kush::compile::cpp::proxy {

class Boolean : public Value {
 public:
  explicit Boolean(CppProgram& program);
  Boolean(CppProgram& program, bool value);

  std::string_view Get() const override;

  void Assign(Boolean& rhs);
  std::unique_ptr<Boolean> operator!();
  std::unique_ptr<Boolean> operator&&(Boolean& rhs);
  std::unique_ptr<Boolean> operator||(Boolean& rhs);
  std::unique_ptr<Boolean> operator==(Boolean& rhs);
  std::unique_ptr<Boolean> operator!=(Boolean& rhs);

 private:
  CppProgram& program_;
  std::string variable_;
};

}  // namespace kush::compile::cpp::proxy