#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"

namespace kush::compile::cpp::proxy {

class Boolean {
 public:
  Boolean(CppProgram& program, bool value);
  Boolean(CppProgram& program, std::string_view variable);

  void Get();

  Boolean operator!();
  Boolean operator&&(Boolean& rhs);
  Boolean operator||(Boolean& rhs);
  Boolean operator==(Boolean& rhs);
  Boolean operator!=(Boolean& rhs);

 private:
  CppProgram& program_;
  std::variant<bool, std::string> value_;
};

}  // namespace kush::compile::cpp::proxy