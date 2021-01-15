#pragma once

#include <string>
#include <string_view>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class Boolean : public Value {
 public:
  Boolean(ProgramBuilder<T>& program, ProgramBuilder<T>::Value& value);
  Boolean(ProgramBuilder<T>& program, bool value);

  ProgramBuilder<T>::Value& Get() const override { return value_; }

  Boolean operator!();
  Boolean operator&&(const Boolean& rhs);
  Boolean operator||(const Boolean& rhs);
  Boolean operator==(const Boolean& rhs);
  Boolean operator!=(const Boolean& rhs);

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy