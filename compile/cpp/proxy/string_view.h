#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

class StringView {
 public:
  StringView(CppProgram& program);
  StringView(CppProgram& program, std::string_view value);

  std::string_view Get();

  Boolean Contains(StringView& rhs);
  Boolean StartsWith(StringView& rhs);
  Boolean EndsWith(StringView& rhs);
  Boolean operator==(StringView& rhs);
  Boolean operator!=(StringView& rhs);

 private:
  CppProgram& program_;
  std::string variable_;
};

}  // namespace kush::compile::cpp::proxy