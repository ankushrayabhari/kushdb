#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename Impl>
class Int8 : public Value {
 public:
  Int8(ProgramBuilder<Impl>& program, ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  Int8(ProgramBuilder<Impl>& program, int8_t value)
      : program_(program), value_(program.ConstI8(value)) {}

  ProgramBuilder<Impl>::Value& Get() const override { return value_; }

  Int8 operator+(const Int8& rhs) {
    return Int8(program_, program_.AddI8(value_, rhs.value_));
  }

  Int8 operator-(const Int8& rhs) {
    return Int8(program_, program_.SubI8(value_, rhs.value_));
  }

  Int8 operator*(const Int8& rhs) {
    return Int8(program_, program_.MulI8(value_, rhs.value_));
  }

  Int8 operator/(const Int8& rhs) {
    return Int8(program_, program_.DivI8(value_, rhs.value_));
  }

  Boolean operator==(const Int8& rhs) {
    return Boolean(program_,
                   program_.CmpI8(Impl::CompType::ICMP_EQ, value_, rhs.value_));
  }

  Boolean operator!=(const Int8& rhs) {
    return Boolean(
        program_, program_.CmpI8(Impl::CompType::ICMP_NEQ, value_, rhs.value_));
  }

  Boolean operator<(const Int8& rhs) {
    return Boolean(
        program_, program_.CmpI8(Impl::CompType::ICMP_SLT, value_, rhs.value_));
  }

  Boolean operator<=(const Int8& rhs) {
    return Boolean(
        program_, program_.CmpI8(Impl::CompType::ICMP_SLE, value_, rhs.value_));
  }

  Boolean operator>(const Int8& rhs) {
    return Boolean(
        program_, program_.CmpI8(Impl::CompType::ICMP_SGT, value_, rhs.value_));
  }

  Boolean operator>=(const Int8& rhs) {
    return Boolean(
        program_, program_.CmpI8(Impl::CompType::ICMP_SGE, value_, rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

template <typename Impl>
class Int16 : public Value {
 public:
  Int16(ProgramBuilder<Impl>& program, ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  Int16(ProgramBuilder<Impl>& program, int16_t value)
      : program_(program), value_(program.ConstI16(value)) {}

  ProgramBuilder<Impl>::Value& Get() const override { return value_; }

  Int16 operator+(const Int16& rhs) {
    return Int16(program_, program_.AddI16(value_, rhs.value_));
  }

  Int16 operator-(const Int16& rhs) {
    return Int16(program_, program_.SubI16(value_, rhs.value_));
  }

  Int16 operator*(const Int16& rhs) {
    return Int16(program_, program_.MulI16(value_, rhs.value_));
  }

  Int16 operator/(const Int16& rhs) {
    return Int16(program_, program_.DivI16(value_, rhs.value_));
  }

  Boolean operator==(const Int16& rhs) {
    return Boolean(
        program_, program_.CmpI16(Impl::CompType::ICMP_EQ, value_, rhs.value_));
  }

  Boolean operator!=(const Int16& rhs) {
    return Boolean(program_, program_.CmpI16(Impl::CompType::ICMP_NEQ, value_,
                                             rhs.value_));
  }

  Boolean operator<(const Int16& rhs) {
    return Boolean(program_, program_.CmpI16(Impl::CompType::ICMP_SLT, value_,
                                             rhs.value_));
  }

  Boolean operator<=(const Int16& rhs) {
    return Boolean(program_, program_.CmpI16(Impl::CompType::ICMP_SLE, value_,
                                             rhs.value_));
  }

  Boolean operator>(const Int16& rhs) {
    return Boolean(program_, program_.CmpI16(Impl::CompType::ICMP_SGT, value_,
                                             rhs.value_));
  }

  Boolean operator>=(const Int16& rhs) {
    return Boolean(program_, program_.CmpI16(Impl::CompType::ICMP_SGE, value_,
                                             rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

template <typename Impl>
class Int32 : public Value {
 public:
  Int32(ProgramBuilder<Impl>& program, ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  Int32(ProgramBuilder<Impl>& program, int32_t value)
      : program_(program), value_(program.ConstI32(value)) {}

  ProgramBuilder<Impl>::Value& Get() const override { return value_; }

  Int32 operator+(const Int32& rhs) {
    return Int32(program_, program_.AddI32(value_, rhs.value_));
  }

  Int32 operator-(const Int32& rhs) {
    return Int32(program_, program_.SubI32(value_, rhs.value_));
  }

  Int32 operator*(const Int32& rhs) {
    return Int32(program_, program_.MulI32(value_, rhs.value_));
  }

  Int32 operator/(const Int32& rhs) {
    return Int32(program_, program_.DivI32(value_, rhs.value_));
  }

  Boolean operator==(const Int32& rhs) {
    return Boolean(
        program_, program_.CmpI32(Impl::CompType::ICMP_EQ, value_, rhs.value_));
  }

  Boolean operator!=(const Int32& rhs) {
    return Boolean(program_, program_.CmpI32(Impl::CompType::ICMP_NEQ, value_,
                                             rhs.value_));
  }

  Boolean operator<(const Int32& rhs) {
    return Boolean(program_, program_.CmpI32(Impl::CompType::ICMP_SLT, value_,
                                             rhs.value_));
  }

  Boolean operator<=(const Int32& rhs) {
    return Boolean(program_, program_.CmpI32(Impl::CompType::ICMP_SLE, value_,
                                             rhs.value_));
  }

  Boolean operator>(const Int32& rhs) {
    return Boolean(program_, program_.CmpI32(Impl::CompType::ICMP_SGT, value_,
                                             rhs.value_));
  }

  Boolean operator>=(const Int32& rhs) {
    return Boolean(program_, program_.CmpI32(Impl::CompType::ICMP_SGE, value_,
                                             rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

template <typename Impl>
class Int64 : public Value {
 public:
  Int64(ProgramBuilder<Impl>& program, ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  Int64(ProgramBuilder<Impl>& program, int64_t value)
      : program_(program), value_(program.ConstI64(value)) {}

  ProgramBuilder<Impl>::Value& Get() const override { return value_; }

  Int64 operator+(const Int64& rhs) {
    return Int64(program_, program_.AddI64(value_, rhs.value_));
  }

  Int64 operator-(const Int64& rhs) {
    return Int64(program_, program_.SubI64(value_, rhs.value_));
  }

  Int64 operator*(const Int64& rhs) {
    return Int64(program_, program_.MulI64(value_, rhs.value_));
  }

  Int64 operator/(const Int64& rhs) {
    return Int64(program_, program_.DivI64(value_, rhs.value_));
  }

  Boolean operator==(const Int64& rhs) {
    return Boolean(
        program_, program_.CmpI64(Impl::CompType::ICMP_EQ, value_, rhs.value_));
  }

  Boolean operator!=(const Int64& rhs) {
    return Boolean(program_, program_.CmpI64(Impl::CompType::ICMP_NEQ, value_,
                                             rhs.value_));
  }

  Boolean operator<(const Int64& rhs) {
    return Boolean(program_, program_.CmpI64(Impl::CompType::ICMP_SLT, value_,
                                             rhs.value_));
  }

  Boolean operator<=(const Int64& rhs) {
    return Boolean(program_, program_.CmpI64(Impl::CompType::ICMP_SLE, value_,
                                             rhs.value_));
  }

  Boolean operator>(const Int64& rhs) {
    return Boolean(program_, program_.CmpI64(Impl::CompType::ICMP_SGT, value_,
                                             rhs.value_));
  }

  Boolean operator>=(const Int64& rhs) {
    return Boolean(program_, program_.CmpI64(Impl::CompType::ICMP_SGE, value_,
                                             rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy