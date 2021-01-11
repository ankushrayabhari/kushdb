#include "compile/cpp/proxy/string_view.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

StringView::StringView(CppProgram& program, std::string_view value_or_variable,
                       bool is_value)
    : program_(program),
      is_value_(is_value),
      value_or_variable_(std::string(value_or_variable)) {}

void StringView::Get() {
  if (is_value_) {
    program_.fout << "\"" << value_or_variable_ << "\"";
  } else {
    program_.fout << value_or_variable_;
  }
}

Boolean StringView::Contains(StringView& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << ".contains(";
  rhs.Get();
  program_.fout << ");";
  return result;
}

Boolean StringView::StartsWith(StringView& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << ".starts_with(";
  rhs.Get();
  program_.fout << ");";
  return result;
}

Boolean StringView::EndsWith(StringView& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << ".ends_with(";
  rhs.Get();
  program_.fout << ");";
  return result;
}

Boolean StringView::operator==(StringView& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean StringView::operator!=(StringView& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy