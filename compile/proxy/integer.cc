#include "compile/proxy/integer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::proxy {

Int8::Int8(CppProgram& program)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int8_t " << value_ << ";";
}

Int8::Int8(CppProgram& program, int8_t value)
    : program_(program), value_(program.GenerateVariable()) {
  program_.fout << "int8_t " << value_ << "(" << value << ");";
}

std::string_view Int8::Get() const { return value_; }

void Int8::Assign(Int8& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Int8> Int8::operator+(Int8& rhs) {
  auto result = std::make_unique<Int8>(program_);
  program_.fout << result->Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int8> Int8::operator-(Int8& rhs) {
  auto result = std::make_unique<Int8>(program_);
  program_.fout << result->Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int8> Int8::operator*(Int8& rhs) {
  auto result = std::make_unique<Int8>(program_);
  program_.fout << result->Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int8> Int8::operator/(Int8& rhs) {
  auto result = std::make_unique<Int8>(program_);
  program_.fout << result->Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator==(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator!=(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator<(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator<=(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " <= " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator>(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int8::operator>=(Int8& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " >= " << rhs.Get()
                << ";";
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

std::string_view Int16::Get() const { return value_; }

void Int16::Assign(Int16& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Int16> Int16::operator+(Int16& rhs) {
  auto result = std::make_unique<Int16>(program_);
  program_.fout << result->Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int16> Int16::operator-(Int16& rhs) {
  auto result = std::make_unique<Int16>(program_);
  program_.fout << result->Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int16> Int16::operator*(Int16& rhs) {
  auto result = std::make_unique<Int16>(program_);
  program_.fout << result->Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int16> Int16::operator/(Int16& rhs) {
  auto result = std::make_unique<Int16>(program_);
  program_.fout << result->Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator==(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator!=(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator<(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator<=(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " <= " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator>(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int16::operator>=(Int16& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " >= " << rhs.Get()
                << ";";
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

std::string_view Int32::Get() const { return value_; }

void Int32::Assign(Int32& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Int32> Int32::operator+(Int32& rhs) {
  auto result = std::make_unique<Int32>(program_);
  program_.fout << result->Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int32> Int32::operator-(Int32& rhs) {
  auto result = std::make_unique<Int32>(program_);
  program_.fout << result->Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int32> Int32::operator*(Int32& rhs) {
  auto result = std::make_unique<Int32>(program_);
  program_.fout << result->Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int32> Int32::operator/(Int32& rhs) {
  auto result = std::make_unique<Int32>(program_);
  program_.fout << result->Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator==(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator!=(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator<(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator<=(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " <= " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator>(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int32::operator>=(Int32& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " >= " << rhs.Get()
                << ";";
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

std::string_view Int64::Get() const { return value_; }

void Int64::Assign(Int64& rhs) {
  program_.fout << Get() << " = " << rhs.Get() << ";";
}

std::unique_ptr<Int64> Int64::operator+(Int64& rhs) {
  auto result = std::make_unique<Int64>(program_);
  program_.fout << result->Get() << " = " << Get() << " + " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int64> Int64::operator-(Int64& rhs) {
  auto result = std::make_unique<Int64>(program_);
  program_.fout << result->Get() << " = " << Get() << " - " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int64> Int64::operator*(Int64& rhs) {
  auto result = std::make_unique<Int64>(program_);
  program_.fout << result->Get() << " = " << Get() << " * " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Int64> Int64::operator/(Int64& rhs) {
  auto result = std::make_unique<Int64>(program_);
  program_.fout << result->Get() << " = " << Get() << " / " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator==(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " == " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator!=(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " != " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator<(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " < " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator<=(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " <= " << rhs.Get()
                << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator>(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " > " << rhs.Get() << ";";
  return result;
}

std::unique_ptr<Boolean> Int64::operator>=(Int64& rhs) {
  auto result = std::make_unique<Boolean>(program_);
  program_.fout << result->Get() << " = " << Get() << " >= " << rhs.Get()
                << ";";
  return result;
}

}  // namespace kush::compile::proxy