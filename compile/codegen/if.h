#pragma once

#include <functional>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::codegen {

class If {
 public:
  template <typename T>
  If(ProgramBuilder<T>& program, proxy::Boolean& boolean,
     std::function<void()> then_fn) {}

  template <typename T>
  If(ProgramBuilder<T>& program, proxy::Boolean& boolean,
     std::function<void()> then_fn, std::function<void()> else_fn) {}
};

}  // namespace kush::compile::codegen