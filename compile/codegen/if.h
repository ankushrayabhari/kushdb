#pragma once

#include <functional>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::codegen {

class If {
 public:
  If(CppProgram& program, proxy::Boolean& boolean,
     std::function<void()> then_fn);
  If(CppProgram& program, proxy::Boolean& boolean,
     std::function<void()> then_fn, std::function<void()> else_fn);
};

}  // namespace kush::compile::codegen