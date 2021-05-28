#pragma once

#include <memory>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

template <typename S>
class Ptr {
 public:
  Ptr(khir::KHIRProgramBuilder& program,
      const khir::Value& value)
      : program_(program), value_(value) {}

  khir::Value Get() const { return value_; }

  void Store(const S& data) { program_.Store(value_, data.Get()); }

  S Load() { return S(program_, program_.Load(value_)); }

 private:
  khir::KHIRProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy