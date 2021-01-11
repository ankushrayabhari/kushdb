#include "compile/cpp/proxy/boolean.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"

namespace kush::compile::cpp::proxy {

Boolean::Boolean(CppProgram& program)
    : program_(program), variable_(program.GenerateVariable()) {
  program_.fout << "bool " << variable_ << ";";
}

Boolean::Boolean(CppProgram& program, bool value)
    : program_(program), variable_(program.GenerateVariable()) {
  program_.fout << "bool " << variable_ << "(" << value << ");";
}

std::string_view Boolean::Get() { return variable_; }

void Boolean::operator=(Boolean& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Boolean> Boolean::operator!() {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = !" << Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Boolean::operator&&(Boolean& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " && " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Boolean::operator||(Boolean& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " || " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Boolean::operator==(Boolean& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Boolean::operator!=(Boolean& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy