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
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << ".contains(";
  rhs.Get();
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::StartsWith(StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << ".starts_with(";
  rhs.Get();
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::EndsWith(StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << ".ends_with(";
  rhs.Get();
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::operator==(StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean StringView::operator!=(StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

}  // namespace kush::compile::cpp::proxy