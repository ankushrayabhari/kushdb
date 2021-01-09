#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

class Int8 {
 public:
  Int8(CppProgram& program, int8_t value);
  Int8(CppProgram& program, std::string_view variable);

  void Get();

  Int8 operator+(Int8& rhs);
  Int8 operator-(Int8& rhs);
  Int8 operator*(Int8& rhs);
  Int8 operator/(Int8& rhs);
  Boolean operator==(Int8& rhs);
  Boolean operator!=(Int8& rhs);
  Boolean operator<(Int8& rhs);
  Boolean operator<=(Int8& rhs);
  Boolean operator>(Int8& rhs);
  Boolean operator>=(Int8& rhs);

 private:
  CppProgram& program_;
  std::variant<int8_t, std::string> value_;
};

class Int16 {
 public:
  Int16(CppProgram& program, int16_t value);
  Int16(CppProgram& program, std::string_view variable);

  void Get();

  Int16 operator+(Int16& rhs);
  Int16 operator-(Int16& rhs);
  Int16 operator*(Int16& rhs);
  Int16 operator/(Int16& rhs);
  Boolean operator==(Int16& rhs);
  Boolean operator!=(Int16& rhs);
  Boolean operator<(Int16& rhs);
  Boolean operator<=(Int16& rhs);
  Boolean operator>(Int16& rhs);
  Boolean operator>=(Int16& rhs);

 private:
  CppProgram& program_;
  std::variant<int16_t, std::string> value_;
};

class Int32 {
 public:
  Int32(CppProgram& program, int32_t value);
  Int32(CppProgram& program, std::string_view variable);

  void Get();

  Int32 operator+(Int32& rhs);
  Int32 operator-(Int32& rhs);
  Int32 operator*(Int32& rhs);
  Int32 operator/(Int32& rhs);
  Boolean operator==(Int32& rhs);
  Boolean operator!=(Int32& rhs);
  Boolean operator<(Int32& rhs);
  Boolean operator<=(Int32& rhs);
  Boolean operator>(Int32& rhs);
  Boolean operator>=(Int32& rhs);

 private:
  CppProgram& program_;
  std::variant<int32_t, std::string> value_;
};

class Int64 {
 public:
  Int64(CppProgram& program, int64_t value);
  Int64(CppProgram& program, std::string_view variable);

  void Get();

  Int64 operator+(Int64& rhs);
  Int64 operator-(Int64& rhs);
  Int64 operator*(Int64& rhs);
  Int64 operator/(Int64& rhs);
  Boolean operator==(Int64& rhs);
  Boolean operator!=(Int64& rhs);
  Boolean operator<(Int64& rhs);
  Boolean operator<=(Int64& rhs);
  Boolean operator>(Int64& rhs);
  Boolean operator>=(Int64& rhs);

 private:
  CppProgram& program_;
  std::variant<int64_t, std::string> value_;
};

}  // namespace kush::compile::cpp::proxy