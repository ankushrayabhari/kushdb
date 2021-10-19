#pragma once

#include <cstdint>

#include "absl/time/civil_time.h"

#include "khir/program_builder.h"

namespace kush::compile::proxy {

class Bool;
class Int64;
class Float64;
class Printer;

class IRValue {
 public:
  virtual ~IRValue() = default;

  virtual Int64 Hash() const = 0;
  virtual khir::Value Get() const = 0;
  virtual void Print(Printer& printer) const = 0;
  virtual khir::ProgramBuilder& ProgramBuilder() const = 0;
};

class Bool : public IRValue {
 public:
  Bool(khir::ProgramBuilder& program, const khir::Value& value);
  Bool(khir::ProgramBuilder& program, bool value);
  Bool(const Bool& rhs) = default;
  Bool(Bool&& rhs) = default;
  Bool& operator=(const Bool& rhs);
  Bool& operator=(Bool&& rhs);

  Bool operator!() const;
  Bool operator==(const Bool& rhs) const;
  Bool operator!=(const Bool& rhs) const;
  Bool operator||(const Bool& rhs) const;
  Bool operator&&(const Bool& rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Bool> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  std::optional<khir::Value> null_value_;
};

class Int8 : public IRValue {
 public:
  Int8(khir::ProgramBuilder& program, const khir::Value& value);
  Int8(khir::ProgramBuilder& program, int8_t value);
  Int8(const Int8& rhs) = default;
  Int8(Int8&& rhs) = default;
  Int8& operator=(const Int8& rhs);
  Int8& operator=(Int8&& rhs);

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
  void Print(Printer& printer) const override;
  std::unique_ptr<Int8> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int16 : public IRValue {
 public:
  Int16(khir::ProgramBuilder& program, const khir::Value& value);
  Int16(khir::ProgramBuilder& program, int16_t value);
  Int16(const Int16& rhs) = default;
  Int16(Int16&& rhs) = default;
  Int16& operator=(const Int16& rhs);
  Int16& operator=(Int16&& rhs);

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
  void Print(Printer& printer) const override;
  std::unique_ptr<Int16> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int32 : public IRValue {
 public:
  Int32(khir::ProgramBuilder& program, const khir::Value& value);
  Int32(khir::ProgramBuilder& program, int32_t value);
  Int32(const Int32& rhs) = default;
  Int32(Int32&& rhs) = default;
  Int32& operator=(const Int32& rhs);
  Int32& operator=(Int32&& rhs);

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
  void Print(Printer& printer) const override;
  std::unique_ptr<Int32> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Int64 : public IRValue {
 public:
  Int64(khir::ProgramBuilder& program, const khir::Value& value);
  Int64(khir::ProgramBuilder& program, int64_t value);
  Int64(const Int64& rhs) = default;
  Int64(Int64&& rhs) = default;
  Int64& operator=(const Int64& rhs);
  Int64& operator=(Int64&& rhs);

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
  void Print(Printer& printer) const override;
  std::unique_ptr<Int64> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Float64 : public IRValue {
 public:
  Float64(khir::ProgramBuilder& program, const khir::Value& value);
  Float64(khir::ProgramBuilder& program, double value);
  Float64(khir::ProgramBuilder& program, const Int8& v);
  Float64(khir::ProgramBuilder& program, const Int16& v);
  Float64(khir::ProgramBuilder& program, const Int32& v);
  Float64(khir::ProgramBuilder& program, const Int64& v);
  Float64(const Float64& rhs) = default;
  Float64(Float64&& rhs) = default;
  Float64& operator=(const Float64& rhs);
  Float64& operator=(Float64&& rhs);

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
  void Print(Printer& printer) const override;
  std::unique_ptr<Float64> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class String : public IRValue {
 public:
  String(khir::ProgramBuilder& program, const khir::Value& value);
  String(const String& rhs) = default;
  String(String&& rhs) = default;
  String& operator=(const String& rhs);
  String& operator=(String&& rhs);

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
  Bool operator<=(const String& rhs) const;
  Bool operator>(const String& rhs) const;
  Bool operator>=(const String& rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<String> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

  static void ForwardDeclare(khir::ProgramBuilder& program);
  static const std::string_view StringStructName;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class Date : public IRValue {
 public:
  Date(khir::ProgramBuilder& program, const khir::Value& value);
  Date(khir::ProgramBuilder& program, absl::CivilDay value);
  Date(const Date& rhs) = default;
  Date(Date&& rhs) = default;
  Date& operator=(const Date& rhs);
  Date& operator=(Date&& rhs);

  Bool operator==(const Date& rhs) const;
  Bool operator!=(const Date& rhs) const;
  Bool operator<(const Date& rhs) const;
  Bool operator<=(const Date& rhs) const;
  Bool operator>(const Date& rhs) const;
  Bool operator>=(const Date& rhs) const;

  Int64 Hash() const override;
  khir::Value Get() const override;
  void Print(Printer& printer) const override;
  std::unique_ptr<Date> ToPointer() const;
  khir::ProgramBuilder& ProgramBuilder() const override;

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
  void Print(const Date& t) const;
  void PrintNewline() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile::proxy