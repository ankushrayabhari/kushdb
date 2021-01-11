#include "compile/cpp/proxy/integer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

Int8::Int8(CppProgram& program, int8_t value)
    : program_(program), value_(value) {}

Int8::Int8(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

void Int8::Get() {
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
}

Int8 Int8::operator+(Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  Get();
  program_.fout << " + ";
  rhs.Get();
  program_.fout << ";";
  return Int8(program_, var);
}

Int8 Int8::operator-(Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  Get();
  program_.fout << " - ";
  rhs.Get();
  program_.fout << ";";
  return Int8(program_, var);
}

Int8 Int8::operator*(Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  Get();
  program_.fout << " * ";
  rhs.Get();
  program_.fout << ";";
  return Int8(program_, var);
}

Int8 Int8::operator/(Int8& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int8_t " << var << " = ";
  Get();
  program_.fout << " / ";
  rhs.Get();
  program_.fout << ";";
  return Int8(program_, var);
}

Boolean Int8::operator==(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int8::operator!=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int8::operator<(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " < ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int8::operator<=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " <= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int8::operator>(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " > ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int8::operator>=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " >= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

//======================================================================

Int16::Int16(CppProgram& program, int16_t value)
    : program_(program), value_(value) {}

Int16::Int16(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

void Int16::Get() {
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
}

Int16 Int16::operator+(Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  Get();
  program_.fout << " + ";
  rhs.Get();
  program_.fout << ";";
  return Int16(program_, var);
}

Int16 Int16::operator-(Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  Get();
  program_.fout << " - ";
  rhs.Get();
  program_.fout << ";";
  return Int16(program_, var);
}

Int16 Int16::operator*(Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  Get();
  program_.fout << " * ";
  rhs.Get();
  program_.fout << ";";
  return Int16(program_, var);
}

Int16 Int16::operator/(Int16& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int16_t " << var << " = ";
  Get();
  program_.fout << " / ";
  rhs.Get();
  program_.fout << ";";
  return Int16(program_, var);
}

Boolean Int16::operator==(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int16::operator!=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int16::operator<(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " < ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int16::operator<=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " <= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int16::operator>(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " > ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int16::operator>=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " >= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

//======================================================================

Int32::Int32(CppProgram& program, int32_t value)
    : program_(program), value_(value) {}

Int32::Int32(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

void Int32::Get() {
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
}

Int32 Int32::operator+(Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  Get();
  program_.fout << " + ";
  rhs.Get();
  program_.fout << ";";
  return Int32(program_, var);
}

Int32 Int32::operator-(Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  Get();
  program_.fout << " - ";
  rhs.Get();
  program_.fout << ";";
  return Int32(program_, var);
}

Int32 Int32::operator*(Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  Get();
  program_.fout << " * ";
  rhs.Get();
  program_.fout << ";";
  return Int32(program_, var);
}

Int32 Int32::operator/(Int32& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int32_t " << var << " = ";
  Get();
  program_.fout << " / ";
  rhs.Get();
  program_.fout << ";";
  return Int32(program_, var);
}

Boolean Int32::operator==(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int32::operator!=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int32::operator<(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " < ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int32::operator<=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " <= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int32::operator>(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " > ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int32::operator>=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " >= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

//======================================================================

Int64::Int64(CppProgram& program, int64_t value)
    : program_(program), value_(value) {}

Int64::Int64(CppProgram& program, std::string_view variable)
    : program_(program), value_(std::string(variable)) {}

void Int64::Get() {
  std::visit([this](auto&& arg) { program_.fout << arg; }, value_);
}

Int64 Int64::operator+(Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  Get();
  program_.fout << " + ";
  rhs.Get();
  program_.fout << ";";
  return Int64(program_, var);
}

Int64 Int64::operator-(Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  Get();
  program_.fout << " - ";
  rhs.Get();
  program_.fout << ";";
  return Int64(program_, var);
}

Int64 Int64::operator*(Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  Get();
  program_.fout << " * ";
  rhs.Get();
  program_.fout << ";";
  return Int64(program_, var);
}

Int64 Int64::operator/(Int64& rhs) {
  auto var = program_.GenerateVariable();
  program_.fout << "int64_t " << var << " = ";
  Get();
  program_.fout << " / ";
  rhs.Get();
  program_.fout << ";";
  return Int64(program_, var);
}

Boolean Int64::operator==(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " == ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int64::operator!=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " != ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int64::operator<(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " < ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int64::operator<=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " <= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int64::operator>(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " > ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

Boolean Int64::operator>=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << "bool " << result.Get() << " = ";
  Get();
  program_.fout << " >= ";
  rhs.Get();
  program_.fout << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy