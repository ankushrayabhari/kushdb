#include "compile/proxy/double.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::proxy {

Double::Double(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "double " << value_ << ";";
}

Double::Double(CppProgram& program, double value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "double " << value_ << "(" << value << ");";
}

std::string_view Double::Get() const { return value_; }

void Double::Assign(Double& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Double> Double::operator+(Double& rhs) {
  auto result = std::make_unique<Double>(program_);
  program_.fout << result->Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Double> Double::operator-(Double& rhs) {
  auto result = std::make_unique<Double>(program_);
  program_.fout << result->Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Double> Double::operator*(Double& rhs) {
  auto result = std::make_unique<Double>(program_);
  program_.fout << result->Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Double> Double::operator/(Double& rhs) {
  auto result = std::make_unique<Double>(program_);
  program_.fout << result->Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator==(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator!=(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator<(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator<=(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " <= " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator>(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Double::operator>=(Double& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " >= " << rhs.Get()
                << ";";
  return result;
}

}  // namespace kush::compile::proxy