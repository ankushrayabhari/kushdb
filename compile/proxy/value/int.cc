#include <cstdint>
#include <memory>

#include "compile/proxy/value/numeric.h"
#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

Int8::Int8(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Int8::Int8(khir::ProgramBuilder& program, int8_t value)
    : program_(program), value_(program.ConstI8(value)) {}

khir::Value Int8::Get() const { return value_; }

Int8 Int8::operator+(const Int8& rhs) const {
  return Int8(program_, program_.AddI8(value_, rhs.value_));
}

Int8 Int8::operator-(const Int8& rhs) const {
  return Int8(program_, program_.SubI8(value_, rhs.value_));
}

Int8 Int8::operator*(const Int8& rhs) const {
  return Int8(program_, program_.MulI8(value_, rhs.value_));
}

Bool Int8::operator==(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::EQ, value_, rhs.value_));
}

Bool Int8::operator!=(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::NE, value_, rhs.value_));
}

Bool Int8::operator<(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::LT, value_, rhs.value_));
}

Bool Int8::operator<=(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::LE, value_, rhs.value_));
}

Bool Int8::operator>(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::GT, value_, rhs.value_));
}

Bool Int8::operator>=(const Int8& rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::GE, value_, rhs.value_));
}

Int8 Int8::operator+(int8_t rhs) const {
  return Int8(program_, program_.AddI8(value_, program_.ConstI8(rhs)));
}

Int8 Int8::operator-(int8_t rhs) const {
  return Int8(program_, program_.SubI8(value_, program_.ConstI8(rhs)));
}

Int8 Int8::operator*(int8_t rhs) const {
  return Int8(program_, program_.MulI8(value_, program_.ConstI8(rhs)));
}

Bool Int8::operator==(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::EQ, value_,
                                       program_.ConstI8(rhs)));
}

Bool Int8::operator!=(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::NE, value_,
                                       program_.ConstI8(rhs)));
}

Bool Int8::operator<(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::LT, value_,
                                       program_.ConstI8(rhs)));
}

Bool Int8::operator<=(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::LE, value_,
                                       program_.ConstI8(rhs)));
}

Bool Int8::operator>(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::GT, value_,
                                       program_.ConstI8(rhs)));
}

Bool Int8::operator>=(int8_t rhs) const {
  return Bool(program_, program_.CmpI8(khir::CompType::GE, value_,
                                       program_.ConstI8(rhs)));
}

std::unique_ptr<Int8> Int8::ToPointer() const {
  return std::make_unique<Int8>(program_, value_);
}

std::unique_ptr<Value> Int8::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, const Value& rhs) const {
  return EvaluateBinaryNumeric<Int8>(op_type, *this, rhs);
}

void Int8::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Int8::Hash() const {
  return proxy::Int64(program_, program_.I64ZextI8(value_));
}

Int16::Int16(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Int16::Int16(khir::ProgramBuilder& program, int16_t value)
    : program_(program), value_(program.ConstI16(value)) {}

khir::Value Int16::Get() const { return value_; }

Int16 Int16::operator+(const Int16& rhs) const {
  return Int16(program_, program_.AddI16(value_, rhs.value_));
}

Int16 Int16::operator-(const Int16& rhs) const {
  return Int16(program_, program_.SubI16(value_, rhs.value_));
}

Int16 Int16::operator*(const Int16& rhs) const {
  return Int16(program_, program_.MulI16(value_, rhs.value_));
}

Bool Int16::operator==(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::EQ, value_, rhs.value_));
}

Bool Int16::operator!=(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::NE, value_, rhs.value_));
}

Bool Int16::operator<(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::LT, value_, rhs.value_));
}

Bool Int16::operator<=(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::LE, value_, rhs.value_));
}

Bool Int16::operator>(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::GT, value_, rhs.value_));
}

Bool Int16::operator>=(const Int16& rhs) const {
  return Bool(program_,
              program_.CmpI16(khir::CompType::GE, value_, rhs.value_));
}

Int16 Int16::operator+(int16_t rhs) const {
  return Int16(program_, program_.AddI16(value_, program_.ConstI16(rhs)));
}

Int16 Int16::operator-(int16_t rhs) const {
  return Int16(program_, program_.SubI16(value_, program_.ConstI16(rhs)));
}

Int16 Int16::operator*(int16_t rhs) const {
  return Int16(program_, program_.MulI16(value_, program_.ConstI16(rhs)));
}

Bool Int16::operator==(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::EQ, value_,
                                        program_.ConstI16(rhs)));
}

Bool Int16::operator!=(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::NE, value_,
                                        program_.ConstI16(rhs)));
}

Bool Int16::operator<(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::LT, value_,
                                        program_.ConstI16(rhs)));
}

Bool Int16::operator<=(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::LE, value_,
                                        program_.ConstI16(rhs)));
}

Bool Int16::operator>(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::GT, value_,
                                        program_.ConstI16(rhs)));
}

Bool Int16::operator>=(int16_t rhs) const {
  return Bool(program_, program_.CmpI16(khir::CompType::GE, value_,
                                        program_.ConstI16(rhs)));
}

std::unique_ptr<Int16> Int16::ToPointer() const {
  return std::make_unique<Int16>(program_, value_);
}

std::unique_ptr<Value> Int16::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, const Value& rhs) const {
  return EvaluateBinaryNumeric<Int16>(op_type, *this, rhs);
}

void Int16::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Int16::Hash() const {
  return proxy::Int64(program_, program_.I64ZextI16(value_));
}

Int32::Int32(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Int32::Int32(khir::ProgramBuilder& program, int32_t value)
    : program_(program), value_(program.ConstI32(value)) {}

khir::Value Int32::Get() const { return value_; }

Int32 Int32::operator+(const Int32& rhs) const {
  return Int32(program_, program_.AddI32(value_, rhs.value_));
}

Int32 Int32::operator-(const Int32& rhs) const {
  return Int32(program_, program_.SubI32(value_, rhs.value_));
}

Int32 Int32::operator*(const Int32& rhs) const {
  return Int32(program_, program_.MulI32(value_, rhs.value_));
}

Bool Int32::operator==(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::EQ, value_, rhs.value_));
}

Bool Int32::operator!=(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::NE, value_, rhs.value_));
}

Bool Int32::operator<(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::LT, value_, rhs.value_));
}

Bool Int32::operator<=(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::LE, value_, rhs.value_));
}

Bool Int32::operator>(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::GT, value_, rhs.value_));
}

Bool Int32::operator>=(const Int32& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::GE, value_, rhs.value_));
}

Int32 Int32::operator+(int32_t rhs) const {
  return Int32(program_, program_.AddI32(value_, program_.ConstI32(rhs)));
}

Int32 Int32::operator-(int32_t rhs) const {
  return Int32(program_, program_.SubI32(value_, program_.ConstI32(rhs)));
}

Int32 Int32::operator*(int32_t rhs) const {
  return Int32(program_, program_.MulI32(value_, program_.ConstI32(rhs)));
}

Bool Int32::operator==(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::EQ, value_,
                                        program_.ConstI32(rhs)));
}

Bool Int32::operator!=(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::NE, value_,
                                        program_.ConstI32(rhs)));
}

Bool Int32::operator<(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::LT, value_,
                                        program_.ConstI32(rhs)));
}

Bool Int32::operator<=(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::LE, value_,
                                        program_.ConstI32(rhs)));
}

Bool Int32::operator>(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::GT, value_,
                                        program_.ConstI32(rhs)));
}

Bool Int32::operator>=(int32_t rhs) const {
  return Bool(program_, program_.CmpI32(khir::CompType::GE, value_,
                                        program_.ConstI32(rhs)));
}

std::unique_ptr<Int32> Int32::ToPointer() const {
  return std::make_unique<Int32>(program_, value_);
}

std::unique_ptr<Value> Int32::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, const Value& rhs) const {
  return EvaluateBinaryNumeric<Int32>(op_type, *this, rhs);
}

void Int32::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Int32::Hash() const {
  return proxy::Int64(program_, program_.I64ZextI32(value_));
}

Int64::Int64(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Int64::Int64(khir::ProgramBuilder& program, int64_t value)
    : program_(program), value_(program.ConstI64(value)) {}

khir::Value Int64::Get() const { return value_; }

Int64 Int64::operator+(const Int64& rhs) const {
  return Int64(program_, program_.AddI64(value_, rhs.value_));
}

Int64 Int64::operator-(const Int64& rhs) const {
  return Int64(program_, program_.SubI64(value_, rhs.value_));
}

Int64 Int64::operator*(const Int64& rhs) const {
  return Int64(program_, program_.MulI64(value_, rhs.value_));
}

Bool Int64::operator==(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::EQ, value_, rhs.value_));
}

Bool Int64::operator!=(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::NE, value_, rhs.value_));
}

Bool Int64::operator<(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::LT, value_, rhs.value_));
}

Bool Int64::operator<=(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::LE, value_, rhs.value_));
}

Bool Int64::operator>(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::GT, value_, rhs.value_));
}

Bool Int64::operator>=(const Int64& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::GE, value_, rhs.value_));
}

Int64 Int64::operator+(int64_t rhs) const {
  return Int64(program_, program_.AddI64(value_, program_.ConstI64(rhs)));
}

Int64 Int64::operator-(int64_t rhs) const {
  return Int64(program_, program_.SubI64(value_, program_.ConstI64(rhs)));
}

Int64 Int64::operator*(int64_t rhs) const {
  return Int64(program_, program_.MulI64(value_, program_.ConstI64(rhs)));
}

Bool Int64::operator==(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::EQ, value_,
                                        program_.ConstI64(rhs)));
}

Bool Int64::operator!=(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::NE, value_,
                                        program_.ConstI64(rhs)));
}

Bool Int64::operator<(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::LT, value_,
                                        program_.ConstI64(rhs)));
}

Bool Int64::operator<=(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::LE, value_,
                                        program_.ConstI64(rhs)));
}

Bool Int64::operator>(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::GT, value_,
                                        program_.ConstI64(rhs)));
}

Bool Int64::operator>=(int64_t rhs) const {
  return Bool(program_, program_.CmpI64(khir::CompType::GE, value_,
                                        program_.ConstI64(rhs)));
}

std::unique_ptr<Int64> Int64::ToPointer() const {
  return std::make_unique<Int64>(program_, value_);
}

std::unique_ptr<Value> Int64::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, const Value& rhs) const {
  return EvaluateBinaryNumeric<Int64>(op_type, *this, rhs);
}

void Int64::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Int64::Hash() const { return *this; }

}  // namespace kush::compile::proxy