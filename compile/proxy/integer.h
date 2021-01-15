#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class Int8 : public Value {
 public:
  explicit Int8(CppProgram& program);
  Int8(CppProgram& program, int8_t value);

  std::string_view Get() const override;

  void Assign(Int8& rhs);
  std::unique_ptr<Int8> operator+(Int8& rhs);
  std::unique_ptr<Int8> operator-(Int8& rhs);
  std::unique_ptr<Int8> operator*(Int8& rhs);
  std::unique_ptr<Int8> operator/(Int8& rhs);
  std::unique_ptr<Boolean> operator==(Int8& rhs);
  std::unique_ptr<Boolean> operator!=(Int8& rhs);
  std::unique_ptr<Boolean> operator<(Int8& rhs);
  std::unique_ptr<Boolean> operator<=(Int8& rhs);
  std::unique_ptr<Boolean> operator>(Int8& rhs);
  std::unique_ptr<Boolean> operator>=(Int8& rhs);

 private:
  CppProgram& program_;
  std::string value_;
};

class Int16 : public Value {
 public:
  explicit Int16(CppProgram& program);
  Int16(CppProgram& program, int16_t value);

  std::string_view Get() const override;

  void Assign(Int16& rhs);
  std::unique_ptr<Int16> operator+(Int16& rhs);
  std::unique_ptr<Int16> operator-(Int16& rhs);
  std::unique_ptr<Int16> operator*(Int16& rhs);
  std::unique_ptr<Int16> operator/(Int16& rhs);
  std::unique_ptr<Boolean> operator==(Int16& rhs);
  std::unique_ptr<Boolean> operator!=(Int16& rhs);
  std::unique_ptr<Boolean> operator<(Int16& rhs);
  std::unique_ptr<Boolean> operator<=(Int16& rhs);
  std::unique_ptr<Boolean> operator>(Int16& rhs);
  std::unique_ptr<Boolean> operator>=(Int16& rhs);

 private:
  CppProgram& program_;
  std::string value_;
};

class Int32 : public Value {
 public:
  explicit Int32(CppProgram& program);
  Int32(CppProgram& program, int32_t value);

  std::string_view Get() const override;

  void Assign(Int32& rhs);
  std::unique_ptr<Int32> operator+(Int32& rhs);
  std::unique_ptr<Int32> operator-(Int32& rhs);
  std::unique_ptr<Int32> operator*(Int32& rhs);
  std::unique_ptr<Int32> operator/(Int32& rhs);
  std::unique_ptr<Boolean> operator==(Int32& rhs);
  std::unique_ptr<Boolean> operator!=(Int32& rhs);
  std::unique_ptr<Boolean> operator<(Int32& rhs);
  std::unique_ptr<Boolean> operator<=(Int32& rhs);
  std::unique_ptr<Boolean> operator>(Int32& rhs);
  std::unique_ptr<Boolean> operator>=(Int32& rhs);

 private:
  CppProgram& program_;
  std::string value_;
};

class Int64 : public Value {
 public:
  explicit Int64(CppProgram& program);
  Int64(CppProgram& program, int64_t value);

  std::string_view Get() const override;

  void Assign(Int64& rhs);
  std::unique_ptr<Int64> operator+(Int64& rhs);
  std::unique_ptr<Int64> operator-(Int64& rhs);
  std::unique_ptr<Int64> operator*(Int64& rhs);
  std::unique_ptr<Int64> operator/(Int64& rhs);
  std::unique_ptr<Boolean> operator==(Int64& rhs);
  std::unique_ptr<Boolean> operator!=(Int64& rhs);
  std::unique_ptr<Boolean> operator<(Int64& rhs);
  std::unique_ptr<Boolean> operator<=(Int64& rhs);
  std::unique_ptr<Boolean> operator>(Int64& rhs);
  std::unique_ptr<Boolean> operator>=(Int64& rhs);

 private:
  CppProgram& program_;
  std::string value_;
};

}  // namespace kush::compile::proxy