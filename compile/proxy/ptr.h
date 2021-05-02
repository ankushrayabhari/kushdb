#pragma once

#include <memory>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename T, typename S>
class Ptr {
 public:
  Ptr(ProgramBuilder<T>& program,
      const typename ProgramBuilder<T>::Value& value)
      : program_(program), value_(value) {}

  typename ProgramBuilder<T>::Value Get() const { return value_; }

  void Store(const S& data) { program_.Store(value_, data.Get()); }

  S Load() { return S(program_, program_.Load(value_)); }

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value value_;
};

}  // namespace kush::compile::proxy