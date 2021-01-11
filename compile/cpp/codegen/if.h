#pragma once

#include <functional>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::codegen {

class If {
 public:
  If(CppProgram& program, proxy::Boolean& boolean,
     std::function<void()> then_fn);
  If(CppProgram& program, proxy::Boolean& boolean,
     std::function<void()> then_fn, std::function<void()> else_fn);
};

}  // namespace kush::compile::cpp::codegen