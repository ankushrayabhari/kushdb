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

Boolean Boolean::operator!() {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = !";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator&&(const Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " && ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator||(const Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " || ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator==(const Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " == ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  program_.fout << ";";
  return Boolean(program_, var);
}

Boolean Boolean::operator!=(const Boolean& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "bool " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " != ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  program_.fout << ";";
  return Boolean(program_, var);
}

}  // namespace kush::compile::cpp::proxy