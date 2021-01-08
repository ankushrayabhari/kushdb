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
  StringView(CppProgram& program, std::string_view value_or_variable,
             bool is_value);

  Boolean Contains(const StringView& rhs);
  Boolean StartsWith(const StringView& rhs);
  Boolean EndsWith(const StringView& rhs);

 private:
  CppProgram& program_;
  bool is_value_;
  std::string value_or_variable_;
};

}  // namespace kush::compile::cpp::proxy