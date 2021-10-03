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

  virtual bool IsNullable() const = 0;
  virtual Bool GetNullableValue() const = 0;
  virtual Int64 Hash() const = 0;

  virtual void Print(Printer& printer) = 0;

  virtual khir::Value Get() const = 0;
  virtual std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& rhs) = 0;
};

class Bool : public Value {
 public:
  Bool(khir::ProgramBuilder& program, const khir::Value& value);
  Bool(khir::ProgramBuilder& program, bool value);

  Bool operator!();
  Bool operator==(const Bool& rhs);
  Bool operator!=(const Bool& rhs);

  Int64 Hash() const override;
  khir::Value Get() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

  std::unique_ptr<Bool> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& rhs) override;
  void Print(Printer& printer) override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  std::optional<khir::Value> null_value_;
};

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
  void Print(Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

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
  void Print(Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

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
  void Print(Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

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
  void Print(Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

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

  khir::Value Get() const override;

  Float64 operator+(const Float64& rhs);
  Float64 operator+(double value);
  Float64 operator-(const Float64& rhs);
  Float64 operator-(double value);
  Float64 operator*(const Float64& rhs);
  Float64 operator*(double value);
  Float64 operator/(const Float64& rhs);
  Float64 operator/(double value);
  Bool operator==(const Float64& rhs);
  Bool operator==(double value);
  Bool operator!=(const Float64& rhs);
  Bool operator!=(double value);
  Bool operator<(const Float64& rhs);
  Bool operator<(double value);
  Bool operator<=(const Float64& rhs);
  Bool operator<=(double value);
  Bool operator>(const Float64& rhs);
  Bool operator>(double value);
  Bool operator>=(const Float64& rhs);
  Bool operator>=(double value);

  std::unique_ptr<Float64> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(proxy::Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;

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

  void Copy(const String& rhs);
  void Reset();

  Bool Contains(const String& rhs);
  Bool StartsWith(const String& rhs);
  Bool EndsWith(const String& rhs);
  Bool Like(const String& rhs);
  Bool operator==(const String& rhs);
  Bool operator!=(const String& rhs);
  Bool operator<(const String& rhs);

  std::unique_ptr<String> ToPointer();
  std::unique_ptr<Value> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value& right_value) override;
  void Print(Printer& printer) override;
  Int64 Hash() const override;
  bool IsNullable() const override;
  Bool GetNullableValue() const override;
  khir::Value Get() const override;

  static void ForwardDeclare(khir::ProgramBuilder& program);
  static const std::string_view StringStructName;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Printer {
 public:
  Printer(khir::ProgramBuilder& program);

  void Print(const Int8& t);
  void Print(const Bool& t);
  void Print(const Int16& t);
  void Print(const Int32& t);
  void Print(const Int64& t);
  void Print(const Float64& t);
  void Print(const String& t);
  void PrintNewline();

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile::proxy