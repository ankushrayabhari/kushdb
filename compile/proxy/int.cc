#include "compile/proxy/int.h"

#include <cstdint>
#include <memory>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/numeric.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T>
Int8<T>::Int8(ProgramBuilder<T>& program,
              typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Int8<T>::Int8(ProgramBuilder<T>& program, int8_t value)
    : program_(program), value_(program.ConstI8(value)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Int8<T>::Get() const {
  return value_;
}

template <typename T>
std::unique_ptr<Int8<T>> Int8<T>::operator+(const Int8<T>& rhs) {
  return std::make_unique<Int8<T>>(program_,
                                   program_.AddI8(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int8<T>> Int8<T>::operator-(const Int8<T>& rhs) {
  return std::make_unique<Int8<T>>(program_,
                                   program_.SubI8(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int8<T>> Int8<T>::operator*(const Int8<T>& rhs) {
  return std::make_unique<Int8<T>>(program_,
                                   program_.MulI8(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int8<T>> Int8<T>::operator/(const Int8<T>& rhs) {
  return std::make_unique<Int8<T>>(program_,
                                   program_.DivI8(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator==(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator!=(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator<(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator<=(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator>(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int8<T>::operator>=(const Int8<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI8(T::CompType::ICMP_SGE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Value<T>> Int8<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  return EvaluateBinaryNumeric<Int8<T>, T>(op_type, *this, rhs);
}

INSTANTIATE_ON_IR(Int8);

template <typename T>
Int16<T>::Int16(ProgramBuilder<T>& program,
                typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Int16<T>::Int16(ProgramBuilder<T>& program, int16_t value)
    : program_(program), value_(program.ConstI16(value)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Int16<T>::Get() const {
  return value_;
}

template <typename T>
std::unique_ptr<Int16<T>> Int16<T>::operator+(const Int16<T>& rhs) {
  return std::make_unique<Int16<T>>(program_,
                                    program_.AddI16(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int16<T>> Int16<T>::operator-(const Int16<T>& rhs) {
  return std::make_unique<Int16<T>>(program_,
                                    program_.SubI16(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int16<T>> Int16<T>::operator*(const Int16<T>& rhs) {
  return std::make_unique<Int16<T>>(program_,
                                    program_.MulI16(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int16<T>> Int16<T>::operator/(const Int16<T>& rhs) {
  return std::make_unique<Int16<T>>(program_,
                                    program_.DivI16(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator==(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator!=(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator<(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator<=(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator>(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int16<T>::operator>=(const Int16<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI16(T::CompType::ICMP_SGE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Value<T>> Int16<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  return EvaluateBinaryNumeric<Int16<T>, T>(op_type, *this, rhs);
}

INSTANTIATE_ON_IR(Int16);

template <typename T>
Int32<T>::Int32(ProgramBuilder<T>& program,
                typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Int32<T>::Int32(ProgramBuilder<T>& program, int32_t value)
    : program_(program), value_(program.ConstI32(value)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Int32<T>::Get() const {
  return value_;
}

template <typename T>
std::unique_ptr<Int32<T>> Int32<T>::operator+(const Int32<T>& rhs) {
  return std::make_unique<Int32<T>>(program_,
                                    program_.AddI32(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int32<T>> Int32<T>::operator-(const Int32<T>& rhs) {
  return std::make_unique<Int32<T>>(program_,
                                    program_.SubI32(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int32<T>> Int32<T>::operator*(const Int32<T>& rhs) {
  return std::make_unique<Int32<T>>(program_,
                                    program_.MulI32(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int32<T>> Int32<T>::operator/(const Int32<T>& rhs) {
  return std::make_unique<Int32<T>>(program_,
                                    program_.DivI32(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator==(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator!=(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator<(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator<=(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator>(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int32<T>::operator>=(const Int32<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI32(T::CompType::ICMP_SGE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Value<T>> Int32<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  return EvaluateBinaryNumeric<Int32<T>, T>(op_type, *this, rhs);
}

INSTANTIATE_ON_IR(Int32);

template <typename T>
Int64<T>::Int64(ProgramBuilder<T>& program,
                typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Int64<T>::Int64(ProgramBuilder<T>& program, int64_t value)
    : program_(program), value_(program.ConstI64(value)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Int64<T>::Get() const {
  return value_;
}

template <typename T>
std::unique_ptr<Int64<T>> Int64<T>::operator+(const Int64<T>& rhs) {
  return std::make_unique<Int64<T>>(program_,
                                    program_.AddI64(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int64<T>> Int64<T>::operator-(const Int64<T>& rhs) {
  return std::make_unique<Int64<T>>(program_,
                                    program_.SubI64(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int64<T>> Int64<T>::operator*(const Int64<T>& rhs) {
  return std::make_unique<Int64<T>>(program_,
                                    program_.MulI64(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Int64<T>> Int64<T>::operator/(const Int64<T>& rhs) {
  return std::make_unique<Int64<T>>(program_,
                                    program_.DivI64(value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator==(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator!=(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator<(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator<=(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator>(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Bool<T>> Int64<T>::operator>=(const Int64<T>& rhs) {
  return std::make_unique<Bool<T>>(
      program_, program_.CmpI64(T::CompType::ICMP_SGE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Value<T>> Int64<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  return EvaluateBinaryNumeric<Int64<T>, T>(op_type, *this, rhs);
}

INSTANTIATE_ON_IR(Int64);

}  // namespace kush::compile::proxy