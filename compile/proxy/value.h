#pragma once

#include "compile/program_builder.h"

namespace kush::compile::proxy {

template <typename T>
class Value {
 public:
  virtual ~Value() = default;

  virtual typename ProgramBuilder<T>::Value& Get() const = 0;
};

}  // namespace kush::compile::proxy