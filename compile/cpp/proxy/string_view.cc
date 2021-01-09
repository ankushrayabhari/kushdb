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

void output(std::ofstream& out, bool is_value,
            std::string_view value_or_variable) {
  if (is_value) {
    out << "\"" << value_or_variable << "\"";
  } else {
    out << value_or_variable;
  }
}

Boolean StringView::Contains(const StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  output(program_.fout, is_value_, value_or_variable_);
  program_.fout << ".contains(";
  output(program_.fout, rhs.is_value_, rhs.value_or_variable_);
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::StartsWith(const StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  output(program_.fout, is_value_, value_or_variable_);
  program_.fout << ".starts_with(";
  output(program_.fout, rhs.is_value_, rhs.value_or_variable_);
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::EndsWith(const StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  output(program_.fout, is_value_, value_or_variable_);
  program_.fout << ".ends_with(";
  output(program_.fout, rhs.is_value_, rhs.value_or_variable_);
  program_.fout << ");";
  return Boolean(program_, var);
}

Boolean StringView::operator==(const StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  output(program_.fout, is_value_, value_or_variable_);
  program_.fout << " == ";
  output(program_.fout, rhs.is_value_, rhs.value_or_variable_);
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean StringView::operator!=(const StringView& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  output(program_.fout, is_value_, value_or_variable_);
  program_.fout << " != ";
  output(program_.fout, rhs.is_value_, rhs.value_or_variable_);
  program_.fout << ";";
  return Boolean(program_, var);
}

}  // namespace kush::compile::cpp::proxy