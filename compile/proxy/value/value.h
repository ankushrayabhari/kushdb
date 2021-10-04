#pragma once

#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class Bool;
class Int64;
class Float64;
class Printer;

class Value {
 public:
  virtual ~Value() = default;

  virtual Int64 Hash() const = 0;
  virtual khir::Value Get() const = 0;
  virtual bool IsNullable() const = 0;
  virtual Bool GetNullableValue() const = 0;
  virtual void Print(Printer& printer) const = 0;
  virtual std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, const Value& rhs) const = 0;
};

class Bool : public Value {
 public:
  Bool(khir::ProgramBuilder& program, const khir::Value& value);
  Bool(khir::ProgramBuilder& program, bool value);

  Bool operator!() const;
  Bool operator==(const Bool& rhs) const;
  Bool operator!=(const Bool& rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;
  std::unique_ptr<Bool> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  std::optional<khir::Value> null_value_;
};

class Int8 : public Value {
 public:
  Int8(khir::ProgramBuilder& program, const khir::Value& value);
  Int8(khir::ProgramBuilder& program, int8_t value);

  Int8 operator+(const Int8& rhs) const;
  Int8 operator+(int8_t rhs) const;
  Int8 operator-(const Int8& rhs) const;
  Int8 operator-(int8_t rhs) const;
  Int8 operator*(const Int8& rhs) const;
  Int8 operator*(int8_t rhs) const;
  Bool operator==(const Int8& rhs) const;
  Bool operator==(int8_t rhs) const;
  Bool operator!=(const Int8& rhs) const;
  Bool operator!=(int8_t rhs) const;
  Bool operator<(const Int8& rhs) const;
  Bool operator<(int8_t rhs) const;
  Bool operator<=(const Int8& rhs) const;
  Bool operator<=(int8_t rhs) const;
  Bool operator>(const Int8& rhs) const;
  Bool operator>(int8_t rhs) const;
  Bool operator>=(const Int8& rhs) const;
  Bool operator>=(int8_t rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<Int8> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int16 : public Value {
 public:
  Int16(khir::ProgramBuilder& program, const khir::Value& value);
  Int16(khir::ProgramBuilder& program, int16_t value);

  Int16 operator+(const Int16& rhs) const;
  Int16 operator+(int16_t rhs) const;
  Int16 operator-(const Int16& rhs) const;
  Int16 operator-(int16_t rhs) const;
  Int16 operator*(const Int16& rhs) const;
  Int16 operator*(int16_t rhs) const;
  Bool operator==(const Int16& rhs) const;
  Bool operator==(int16_t rhs) const;
  Bool operator!=(const Int16& rhs) const;
  Bool operator!=(int16_t rhs) const;
  Bool operator<(const Int16& rhs) const;
  Bool operator<(int16_t rhs) const;
  Bool operator<=(const Int16& rhs) const;
  Bool operator<=(int16_t rhs) const;
  Bool operator>(const Int16& rhs) const;
  Bool operator>(int16_t rhs) const;
  Bool operator>=(const Int16& rhs) const;
  Bool operator>=(int16_t rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<Int16> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int32 : public Value {
 public:
  Int32(khir::ProgramBuilder& program, const khir::Value& value);
  Int32(khir::ProgramBuilder& program, int32_t value);

  Int32 operator+(const Int32& rhs) const;
  Int32 operator+(int32_t rhs) const;
  Int32 operator-(const Int32& rhs) const;
  Int32 operator-(int32_t rhs) const;
  Int32 operator*(const Int32& rhs) const;
  Int32 operator*(int32_t rhs) const;
  Bool operator==(const Int32& rhs) const;
  Bool operator==(int32_t rhs) const;
  Bool operator!=(const Int32& rhs) const;
  Bool operator!=(int32_t rhs) const;
  Bool operator<(const Int32& rhs) const;
  Bool operator<(int32_t rhs) const;
  Bool operator<=(const Int32& rhs) const;
  Bool operator<=(int32_t rhs) const;
  Bool operator>(const Int32& rhs) const;
  Bool operator>(int32_t rhs) const;
  Bool operator>=(const Int32& rhs) const;
  Bool operator>=(int32_t rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<Int32> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int64 : public Value {
 public:
  Int64(khir::ProgramBuilder& program, const khir::Value& value);
  Int64(khir::ProgramBuilder& program, int64_t value);

  Int64 operator+(const Int64& rhs) const;
  Int64 operator+(int64_t rhs) const;
  Int64 operator-(const Int64& rhs) const;
  Int64 operator-(int64_t rhs) const;
  Int64 operator*(const Int64& rhs) const;
  Int64 operator*(int64_t rhs) const;
  Bool operator==(const Int64& rhs) const;
  Bool operator==(int64_t rhs) const;
  Bool operator!=(const Int64& rhs) const;
  Bool operator!=(int64_t rhs) const;
  Bool operator<(const Int64& rhs) const;
  Bool operator<(int64_t rhs) const;
  Bool operator<=(const Int64& rhs) const;
  Bool operator<=(int64_t rhs) const;
  Bool operator>(const Int64& rhs) const;
  Bool operator>(int64_t rhs) const;
  Bool operator>=(const Int64& rhs) const;
  Bool operator>=(int64_t rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<Int64> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Float64 : public Value {
 public:
  Float64(khir::ProgramBuilder& program, const khir::Value& value);
  Float64(khir::ProgramBuilder& program, double value);
  Float64(khir::ProgramBuilder& program, const proxy::Int8& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int16& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int32& v);
  Float64(khir::ProgramBuilder& program, const proxy::Int64& v);

  Float64 operator+(const Float64& rhs) const;
  Float64 operator+(double value) const;
  Float64 operator-(const Float64& rhs) const;
  Float64 operator-(double value) const;
  Float64 operator*(const Float64& rhs) const;
  Float64 operator*(double value) const;
  Float64 operator/(const Float64& rhs) const;
  Float64 operator/(double value) const;
  Bool operator==(const Float64& rhs) const;
  Bool operator==(double value) const;
  Bool operator!=(const Float64& rhs) const;
  Bool operator!=(double value) const;
  Bool operator<(const Float64& rhs) const;
  Bool operator<(double value) const;
  Bool operator<=(const Float64& rhs) const;
  Bool operator<=(double value) const;
  Bool operator>(const Float64& rhs) const;
  Bool operator>(double value) const;
  Bool operator>=(const Float64& rhs) const;
  Bool operator>=(double value) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<Float64> ToPointer() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class String : public Value {
 public:
  String(khir::ProgramBuilder& program, const khir::Value& value);
  static String Global(khir::ProgramBuilder& program, std::string_view value);
  static khir::Value Constant(khir::ProgramBuilder& program,
                              std::string_view value);

  void Copy(const String& rhs) const;
  void Reset() const;
  Bool Contains(const String& rhs) const;
  Bool StartsWith(const String& rhs) const;
  Bool EndsWith(const String& rhs) const;
  Bool Like(const String& rhs) const;
  Bool operator==(const String& rhs) const;
  Bool operator!=(const String& rhs) const;
  Bool operator<(const String& rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type,
      const Value& rhs) const override;

  std::unique_ptr<String> ToPointer() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);
  static const std::string_view StringStructName;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Printer {
 public:
  Printer(khir::ProgramBuilder& program);

  void Print(const Int8& t) const;
  void Print(const Bool& t) const;
  void Print(const Int16& t) const;
  void Print(const Int32& t) const;
  void Print(const Int64& t) const;
  void Print(const Float64& t) const;
  void Print(const String& t) const;
  void PrintNewline() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile::proxy