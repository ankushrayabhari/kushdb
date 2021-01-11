#pragma once

#include <string>
#include <string_view>

#include "compile/cpp/cpp_program.h"

namespace kush::compile::cpp::proxy {

class Boolean {
 public:
  Boolean(CppProgram& program);
  Boolean(CppProgram& program, bool value);

  std::string_view Get();

  Boolean operator!();
  Boolean operator&&(Boolean& rhs);
  Boolean operator||(Boolean& rhs);
  Boolean operator==(Boolean& rhs);
  Boolean operator!=(Boolean& rhs);

 private:
  CppProgram& program_;
  std::string variable_;
};

}  // namespace kush::compile::cpp::proxy