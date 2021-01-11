#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"
#include "compile/cpp/proxy/value.h"

namespace kush::compile::cpp::proxy {

class StringView : public Value {
 public:
  explicit StringView(CppProgram& program);
  StringView(CppProgram& program, std::string_view value);

  std::string_view Get() override;

  void operator=(StringView& rhs);
  std::unique_ptr<Boolean> Contains(StringView& rhs);
  std::unique_ptr<Boolean> StartsWith(StringView& rhs);
  std::unique_ptr<Boolean> EndsWith(StringView& rhs);
  std::unique_ptr<Boolean> operator==(StringView& rhs);
  std::unique_ptr<Boolean> operator!=(StringView& rhs);

 private:
  CppProgram& program_;
  std::string variable_;
};

}  // namespace kush::compile::cpp::proxy