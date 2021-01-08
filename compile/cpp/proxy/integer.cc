#include "compile/cpp/proxy/integer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"

namespace kush::compile::cpp::proxy {

Int8::Int8(CppProgram& program, int8_t value)
    : program_(program), value_(value) {}

Int8::Int8(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

Int8 Int8::operator+(const Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " + ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int8(program_, var);
}

Int8 Int8::operator-(const Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " - ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int8(program_, var);
}

Int8 Int8::operator*(const Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " * ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int8(program_, var);
}

Int8 Int8::operator/(const Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " / ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int8(program_, var);
}

//======================================================================

Int16::Int16(CppProgram& program, int16_t value)
    : program_(program), value_(value) {}

Int16::Int16(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

Int16 Int16::operator+(const Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " + ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int16(program_, var);
}

Int16 Int16::operator-(const Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " - ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int16(program_, var);
}

Int16 Int16::operator*(const Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " * ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int16(program_, var);
}

Int16 Int16::operator/(const Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " / ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int16(program_, var);
}

//======================================================================

Int32::Int32(CppProgram& program, int8_t value)
    : program_(program), value_(value) {}

Int32::Int32(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

Int32 Int32::operator+(const Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " + ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int32(program_, var);
}

Int32 Int32::operator-(const Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " - ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int32(program_, var);
}

Int32 Int32::operator*(const Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " * ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int32(program_, var);
}

Int32 Int32::operator/(const Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " / ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int32(program_, var);
}

//======================================================================

Int64::Int64(CppProgram& program, int8_t value)
    : program_(program), value_(value) {}

Int64::Int64(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

Int64 Int64::operator+(const Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " + ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int64(program_, var);
}

Int64 Int64::operator-(const Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " - ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int64(program_, var);
}

Int64 Int64::operator*(const Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " * ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int64(program_, var);
}

Int64 Int64::operator/(const Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
  program_.fout << " / ";
  std::visit([this](auto&& arg) { program_.fout << arg; }, rhs.value_);
  return Int64(program_, var);
}

}  // namespace kush::compile::cpp::proxy