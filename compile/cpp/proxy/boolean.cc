#include "compile/cpp/proxy/boolean.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"

namespace kush::compile::cpp::proxy {

Boolean::Boolean(CppProgram& program, bool value)
    : program_(program), value_(value) {}

Boolean::Boolean(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

void Boolean::Get() {
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
}

Boolean Boolean::operator!() {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = !";
  Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator&&(Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " && ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator||(Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " || ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator==(Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator!=(Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return Boolean(program_, var);
}

}  // namespace kush::compile::cpp::proxy