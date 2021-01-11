#include "compile/cpp/proxy/string_view.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

StringView::StringView(CppProgram& program)
    : program_(program), variable_(program.GenerateVariable()) {
  program_.fout << "std::string_view " << variable_ << ";";
}

StringView::StringView(CppProgram& program, std::string_view value)
    : program_(program), variable_(program.GenerateVariable()) {
  program_.fout << "std::string_view " << variable_ << "(\"" << value
                << "\"sv);";
}

std::string_view StringView::Get() { return variable_; }

void StringView::operator=(StringView& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Boolean> StringView::Contains(StringView& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << ".contains(" << rhs.Get()
                << ");";
  return result;
}

std::unique_ptr<Boolean> StringView::StartsWith(StringView& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << ".starts_with("
                << rhs.Get() << ");";
  return result;
}

std::unique_ptr<Boolean> StringView::EndsWith(StringView& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << ".ends_with(" << rhs.Get()
                << ");";
  return result;
}

std::unique_ptr<Boolean> StringView::operator==(StringView& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> StringView::operator!=(StringView& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy