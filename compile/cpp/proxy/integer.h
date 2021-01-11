#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::proxy {

class Int8 {
 public:
  Int8(CppProgram& program);
  Int8(CppProgram& program, int8_t value);

  std::string_view Get();

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
  std::string value_;
};

class Int16 {
 public:
  Int16(CppProgram& program);
  Int16(CppProgram& program, int16_t value);

  std::string_view Get();

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
  std::string value_;
};

class Int32 {
 public:
  Int32(CppProgram& program);
  Int32(CppProgram& program, int32_t value);

  std::string_view Get();

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
  std::string value_;
};

class Int64 {
 public:
  Int64(CppProgram& program);
  Int64(CppProgram& program, int64_t value);

  std::string_view Get();

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
  std::string value_;
};

}  // namespace kush::compile::cpp::proxy