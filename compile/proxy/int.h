#pragma once

#include <cstdint>
#include <memory>

#include "compile/proxy/bool.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class Int8 : public Value {
 public:
  Int8(khir::ProgramBuilder& program, const khir::Value& value);
  Int8(khir::ProgramBuilder& program, int8_t value);

  khir::Value Get() const override;

  Int8 operator+(const Int8& rhs);
  Int8 operator+(int8_t rhs);
  Int8 operator-(const Int8& rhs);
  Int8 operator-(int8_t rhs);
  Int8 operator*(const Int8& rhs);
  Int8 operator*(int8_t rhs);
  Bool operator==(const Int8& rhs);
  Bool operator==(int8_t rhs);
  Bool operator!=(const Int8& rhs);
  Bool operator!=(int8_t rhs);
  Bool operator<(const Int8& rhs);
  Bool operator<(int8_t rhs);
  Bool operator<=(const Int8& rhs);
  Bool operator<=(int8_t rhs);
  Bool operator>(const Int8& rhs);
  Bool operator>(int8_t rhs);
  Bool operator>=(const Int8& rhs);
  Bool operator>=(int8_t rhs);

  std::unique_ptr<Int8> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int16 : public Value {
 public:
  Int16(khir::ProgramBuilder& program, const khir::Value& value);
  Int16(khir::ProgramBuilder& program, int16_t value);

  khir::Value Get() const override;

  Int16 operator+(const Int16& rhs);
  Int16 operator+(int16_t rhs);
  Int16 operator-(const Int16& rhs);
  Int16 operator-(int16_t rhs);
  Int16 operator*(const Int16& rhs);
  Int16 operator*(int16_t rhs);
  Bool operator==(const Int16& rhs);
  Bool operator==(int16_t rhs);
  Bool operator!=(const Int16& rhs);
  Bool operator!=(int16_t rhs);
  Bool operator<(const Int16& rhs);
  Bool operator<(int16_t rhs);
  Bool operator<=(const Int16& rhs);
  Bool operator<=(int16_t rhs);
  Bool operator>(const Int16& rhs);
  Bool operator>(int16_t rhs);
  Bool operator>=(const Int16& rhs);
  Bool operator>=(int16_t rhs);

  std::unique_ptr<Int16> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int32 : public Value {
 public:
  Int32(khir::ProgramBuilder& program, const khir::Value& value);
  Int32(khir::ProgramBuilder& program, int32_t value);

  khir::Value Get() const override;

  Int32 operator+(const Int32& rhs);
  Int32 operator+(int32_t rhs);
  Int32 operator-(const Int32& rhs);
  Int32 operator-(int32_t rhs);
  Int32 operator*(const Int32& rhs);
  Int32 operator*(int32_t rhs);
  Bool operator==(const Int32& rhs);
  Bool operator==(int32_t rhs);
  Bool operator!=(const Int32& rhs);
  Bool operator!=(int32_t rhs);
  Bool operator<(const Int32& rhs);
  Bool operator<(int32_t rhs);
  Bool operator<=(const Int32& rhs);
  Bool operator<=(int32_t rhs);
  Bool operator>(const Int32& rhs);
  Bool operator>(int32_t rhs);
  Bool operator>=(const Int32& rhs);
  Bool operator>=(int32_t rhs);

  std::unique_ptr<Int32> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int64 : public Value {
 public:
  Int64(khir::ProgramBuilder& program, const khir::Value& value);
  Int64(khir::ProgramBuilder& program, int64_t value);

  khir::Value Get() const override;

  Int64 operator+(const Int64& rhs);
  Int64 operator+(int64_t rhs);
  Int64 operator-(const Int64& rhs);
  Int64 operator-(int64_t rhs);
  Int64 operator*(const Int64& rhs);
  Int64 operator*(int64_t rhs);
  Bool operator==(const Int64& rhs);
  Bool operator==(int64_t rhs);
  Bool operator!=(const Int64& rhs);
  Bool operator!=(int64_t rhs);
  Bool operator<(const Int64& rhs);
  Bool operator<(int64_t rhs);
  Bool operator<=(const Int64& rhs);
  Bool operator<=(int64_t rhs);
  Bool operator>(const Int64& rhs);
  Bool operator>(int64_t rhs);
  Bool operator>=(const Int64& rhs);
  Bool operator>=(int64_t rhs);

  std::unique_ptr<Int64> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  khir::Value Hash() override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy