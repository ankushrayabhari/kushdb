#include "compile/proxy/int.h"

#include <cstdint>

#include "compile/program_builder.h"
#include "compile/program_registry.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

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
Int8<T> Int8<T>::operator+(const Int8<T>& rhs) {
  return Int8(program_, program_.AddI8(value_, rhs.value_));
}

template <typename T>
Int8<T> Int8<T>::operator-(const Int8<T>& rhs) {
  return Int8(program_, program_.SubI8(value_, rhs.value_));
}

template <typename T>
Int8<T> Int8<T>::operator*(const Int8<T>& rhs) {
  return Int8(program_, program_.MulI8(value_, rhs.value_));
}

template <typename T>
Int8<T> Int8<T>::operator/(const Int8<T>& rhs) {
  return Int8(program_, program_.DivI8(value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator==(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator!=(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator<(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator<=(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator>(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int8<T>::operator>=(const Int8<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI8(T::CompType::ICMP_SGE, value_, rhs.value_));
}

INSTANTIATE_ON_BACKENDS(Int8);

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
Int16<T> Int16<T>::operator+(const Int16<T>& rhs) {
  return Int16(program_, program_.AddI16(value_, rhs.value_));
}

template <typename T>
Int16<T> Int16<T>::operator-(const Int16<T>& rhs) {
  return Int16(program_, program_.SubI16(value_, rhs.value_));
}

template <typename T>
Int16<T> Int16<T>::operator*(const Int16<T>& rhs) {
  return Int16(program_, program_.MulI16(value_, rhs.value_));
}

template <typename T>
Int16<T> Int16<T>::operator/(const Int16<T>& rhs) {
  return Int16(program_, program_.DivI16(value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator==(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator!=(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator<(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator<=(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator>(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int16<T>::operator>=(const Int16<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI16(T::CompType::ICMP_SGE, value_, rhs.value_));
}

INSTANTIATE_ON_BACKENDS(Int16);

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
Int32<T> Int32<T>::operator+(const Int32<T>& rhs) {
  return Int32(program_, program_.AddI32(value_, rhs.value_));
}

template <typename T>
Int32<T> Int32<T>::operator-(const Int32<T>& rhs) {
  return Int32(program_, program_.SubI32(value_, rhs.value_));
}

template <typename T>
Int32<T> Int32<T>::operator*(const Int32<T>& rhs) {
  return Int32(program_, program_.MulI32(value_, rhs.value_));
}

template <typename T>
Int32<T> Int32<T>::operator/(const Int32<T>& rhs) {
  return Int32(program_, program_.DivI32(value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator==(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator!=(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator<(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator<=(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator>(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int32<T>::operator>=(const Int32<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI32(T::CompType::ICMP_SGE, value_, rhs.value_));
}

INSTANTIATE_ON_BACKENDS(Int32);

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
Int64<T> Int64<T>::operator+(const Int64<T>& rhs) {
  return Int64(program_, program_.AddI64(value_, rhs.value_));
}

template <typename T>
Int64<T> Int64<T>::operator-(const Int64<T>& rhs) {
  return Int64(program_, program_.SubI64(value_, rhs.value_));
}

template <typename T>
Int64<T> Int64<T>::operator*(const Int64<T>& rhs) {
  return Int64(program_, program_.MulI64(value_, rhs.value_));
}

template <typename T>
Int64<T> Int64<T>::operator/(const Int64<T>& rhs) {
  return Int64(program_, program_.DivI64(value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator==(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator!=(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_NE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator<(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_SLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator<=(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_SLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator>(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_SGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Int64<T>::operator>=(const Int64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpI64(T::CompType::ICMP_SGE, value_, rhs.value_));
}

INSTANTIATE_ON_BACKENDS(Int64);

}  // namespace kush::compile::proxy