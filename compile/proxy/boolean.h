#pragma once

#include <string>
#include <string_view>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {
template <typename T>
class Boolean : public Value {
 public:
  Boolean(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Boolean(ProgramBuilder<T>& program, bool value);

  ProgramBuilder<T>::Value& Get() const override { return value_; }

  Boolean operator!() { return Boolean(program_, program_.LNotI8(value_)); }

  Boolean operator==(const Boolean& rhs) {
    return Boolean(program_,
                   program_.CmpI8(Impl::CompType::ICMP_EQ, value_, rhs.value_));
  }

  Boolean operator!=(const Boolean& rhs) {
    return Boolean(program_,
                   program_.CmpI8(Impl::CompType::ICMP_NE, value_, rhs.value_));
  }

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy