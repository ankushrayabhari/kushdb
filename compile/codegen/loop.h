#pragma once

#include <functional>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::codegen {

class Loop {
 public:
  template <typename T>
  Loop(ProgramBuilder<T>& program, std::function<void()> init,
       std::function<proxy::Boolean()> cond, std::function<void()> body) {}
};

}  // namespace kush::compile::codegen