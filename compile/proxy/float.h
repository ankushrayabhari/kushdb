#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/boolean.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename Impl>
class Float64 : public Value {
 public:
  Float64(ProgramBuilder<Impl>& program, ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  Float64(ProgramBuilder<Impl>& program, double value)
      : program_(program), value_(program.ConstF64(value)) {}

  ProgramBuilder<Impl>::Value& Get() const override { return value_; }

  Float64 operator+(const Float64& rhs) {
    return Float64(program_, program_.AddF64(value_, rhs.value_));
  }

  Float64 operator-(const Float64& rhs) {
    return Float64(program_, program_.SubF64(value_, rhs.value_));
  }

  Float64 operator*(const Float64& rhs) {
    return Float64(program_, program_.MulF64(value_, rhs.value_));
  }

  Float64 operator/(const Float64& rhs) {
    return Float64(program_, program_.DivF64(value_, rhs.value_));
  }

  Boolean operator==(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_OEQ, value_,
                                             rhs.value_));
  }

  Boolean operator!=(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_ONE, value_,
                                             rhs.value_));
  }

  Boolean operator<(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_OLT, value_,
                                             rhs.value_));
  }

  Boolean operator<=(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_OLE, value_,
                                             rhs.value_));
  }

  Boolean operator>(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_OGT, value_,
                                             rhs.value_));
  }

  Boolean operator>=(const Float64& rhs) {
    return Boolean(program_, program_.CmpF64(Impl::CompType::FCMP_OGE, value_,
                                             rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy