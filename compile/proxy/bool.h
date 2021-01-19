#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class Bool : public Value<T> {
 public:
  Bool(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Bool(ProgramBuilder<T>& program, bool value);

  typename ProgramBuilder<T>::Value& Get() const override;
  Bool operator!();
  Bool operator==(const Bool& rhs);
  Bool operator!=(const Bool& rhs);

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy