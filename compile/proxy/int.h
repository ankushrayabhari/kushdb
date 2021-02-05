#pragma once

#include <cstdint>
#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
class Int8 : public Value<T> {
 public:
  Int8(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Int8(ProgramBuilder<T>& program, int8_t value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Int8<T> operator+(const Int8<T>& rhs);
  Int8<T> operator-(const Int8<T>& rhs);
  Int8<T> operator*(const Int8<T>& rhs);
  Int8<T> operator/(const Int8<T>& rhs);
  Bool<T> operator==(const Int8<T>& rhs);
  Bool<T> operator!=(const Int8<T>& rhs);
  Bool<T> operator<(const Int8<T>& rhs);
  Bool<T> operator<=(const Int8<T>& rhs);
  Bool<T> operator>(const Int8<T>& rhs);
  Bool<T> operator>=(const Int8<T>& rhs);

  std::unique_ptr<Int8<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

template <typename T>
class Int16 : public Value<T> {
 public:
  Int16(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Int16(ProgramBuilder<T>& program, int16_t value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Int16<T> operator+(const Int16<T>& rhs);
  Int16<T> operator-(const Int16<T>& rhs);
  Int16<T> operator*(const Int16<T>& rhs);
  Int16<T> operator/(const Int16<T>& rhs);
  Bool<T> operator==(const Int16<T>& rhs);
  Bool<T> operator!=(const Int16<T>& rhs);
  Bool<T> operator<(const Int16<T>& rhs);
  Bool<T> operator<=(const Int16<T>& rhs);
  Bool<T> operator>(const Int16<T>& rhs);
  Bool<T> operator>=(const Int16<T>& rhs);

  std::unique_ptr<Int16<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

template <typename T>
class Int32 : public Value<T> {
 public:
  Int32(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Int32(ProgramBuilder<T>& program, int32_t value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Int32<T> operator+(const Int32<T>& rhs);
  Int32<T> operator-(const Int32<T>& rhs);
  Int32<T> operator*(const Int32<T>& rhs);
  Int32<T> operator/(const Int32<T>& rhs);
  Bool<T> operator==(const Int32<T>& rhs);
  Bool<T> operator!=(const Int32<T>& rhs);
  Bool<T> operator<(const Int32<T>& rhs);
  Bool<T> operator<=(const Int32<T>& rhs);
  Bool<T> operator>(const Int32<T>& rhs);
  Bool<T> operator>=(const Int32<T>& rhs);

  std::unique_ptr<Int32<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

template <typename T>
class UInt32 : public Value<T> {
 public:
  UInt32(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  UInt32(ProgramBuilder<T>& program, uint32_t value);

  typename ProgramBuilder<T>::Value& Get() const override;

  UInt32<T> operator+(const UInt32<T>& rhs);
  UInt32<T> operator-(const UInt32<T>& rhs);
  UInt32<T> operator*(const UInt32<T>& rhs);
  UInt32<T> operator/(const UInt32<T>& rhs);
  Bool<T> operator==(const UInt32<T>& rhs);
  Bool<T> operator!=(const UInt32<T>& rhs);
  Bool<T> operator<(const UInt32<T>& rhs);
  Bool<T> operator<=(const UInt32<T>& rhs);
  Bool<T> operator>(const UInt32<T>& rhs);
  Bool<T> operator>=(const UInt32<T>& rhs);

  std::unique_ptr<UInt32<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

template <typename T>
class Int64 : public Value<T> {
 public:
  Int64(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Int64(ProgramBuilder<T>& program, int64_t value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Int64<T> operator+(const Int64<T>& rhs);
  Int64<T> operator-(const Int64<T>& rhs);
  Int64<T> operator*(const Int64<T>& rhs);
  Int64<T> operator/(const Int64<T>& rhs);
  Bool<T> operator==(const Int64<T>& rhs);
  Bool<T> operator!=(const Int64<T>& rhs);
  Bool<T> operator<(const Int64<T>& rhs);
  Bool<T> operator<=(const Int64<T>& rhs);
  Bool<T> operator>(const Int64<T>& rhs);
  Bool<T> operator>=(const Int64<T>& rhs);

  std::unique_ptr<Int64<T>> ToPointer();
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      Value<T>& right_value) override;

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy