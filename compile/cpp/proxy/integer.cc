#include "compile/cpp/proxy/integer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

Int8::Int8(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int8_t " << value_ << ";";
}

Int8::Int8(CppProgram& program, int8_t value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int8_t " << value_ << "(" << value << ");";
}

std::string_view Int8::Get() { return value_; }

Int8 Int8::operator+(Int8& rhs) {
  Int8 result(program_);
  program_.fout << result.Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

Int8 Int8::operator-(Int8& rhs) {
  Int8 result(program_);
  program_.fout << result.Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

Int8 Int8::operator*(Int8& rhs) {
  Int8 result(program_);
  program_.fout << result.Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

Int8 Int8::operator/(Int8& rhs) {
  Int8 result(program_);
  program_.fout << result.Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator==(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " == " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator!=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " != " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator<(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator<=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " <= " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator>(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

Boolean Int8::operator>=(Int8& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " >= " << rhs.Get() << ";";
  return result;
}

//======================================================================

Int16::Int16(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int16_t " << value_ << ";";
}

Int16::Int16(CppProgram& program, int16_t value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int16_t " << value_ << "(" << value << ");";
}

std::string_view Int16::Get() { return value_; }

Int16 Int16::operator+(Int16& rhs) {
  Int16 result(program_);
  program_.fout << result.Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

Int16 Int16::operator-(Int16& rhs) {
  Int16 result(program_);
  program_.fout << result.Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

Int16 Int16::operator*(Int16& rhs) {
  Int16 result(program_);
  program_.fout << result.Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

Int16 Int16::operator/(Int16& rhs) {
  Int16 result(program_);
  program_.fout << result.Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator==(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " == " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator!=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " != " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator<(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator<=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " <= " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator>(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

Boolean Int16::operator>=(Int16& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " >= " << rhs.Get() << ";";
  return result;
}

//======================================================================

Int32::Int32(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int32_t " << value_ << ";";
}

Int32::Int32(CppProgram& program, int32_t value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int32_t " << value_ << "(" << value << ");";
}

std::string_view Int32::Get() { return value_; }

Int32 Int32::operator+(Int32& rhs) {
  Int32 result(program_);
  program_.fout << result.Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

Int32 Int32::operator-(Int32& rhs) {
  Int32 result(program_);
  program_.fout << result.Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

Int32 Int32::operator*(Int32& rhs) {
  Int32 result(program_);
  program_.fout << result.Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

Int32 Int32::operator/(Int32& rhs) {
  Int32 result(program_);
  program_.fout << result.Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator==(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " == " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator!=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " != " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator<(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator<=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " <= " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator>(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

Boolean Int32::operator>=(Int32& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " >= " << rhs.Get() << ";";
  return result;
}

//======================================================================

Int64::Int64(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int64_t " << value_ << ";";
}

Int64::Int64(CppProgram& program, int64_t value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int64_t " << value_ << "(" << value << ");";
}

std::string_view Int64::Get() { return value_; }

Int64 Int64::operator+(Int64& rhs) {
  Int64 result(program_);
  program_.fout << result.Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

Int64 Int64::operator-(Int64& rhs) {
  Int64 result(program_);
  program_.fout << result.Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

Int64 Int64::operator*(Int64& rhs) {
  Int64 result(program_);
  program_.fout << result.Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

Int64 Int64::operator/(Int64& rhs) {
  Int64 result(program_);
  program_.fout << result.Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator==(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " == " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator!=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " != " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator<(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator<=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " <= " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator>(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

Boolean Int64::operator>=(Int64& rhs) {
  Boolean result(program_);
  program_.fout << result.Get() << " = " << Get() << " >= " << rhs.Get() << ";";
  return result;
}

}  // namespace kush::compile::cpp::proxy