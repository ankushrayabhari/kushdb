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

Boolean Boolean::operator!() {
  Boolean result(program_);
  program_.fout << result.Get() << " = !" << Get() << ";";
  return result;
}

Boolean Boolean::operator&&(Boolean& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " && " << rhs.Get() << ";";
  return result;
}

Boolean Boolean::operator||(Boolean& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " || " << rhs.Get() << ";";
  return result;
}

Boolean Boolean::operator==(Boolean& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " == " << rhs.Get() << ";";
  return result;
}

Boolean Boolean::operator!=(Boolean& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " != " << rhs.Get() << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy