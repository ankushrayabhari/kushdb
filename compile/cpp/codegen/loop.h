#pragma once

#include <functional>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::codegen {

class Loop {
 public:
  Loop(CppProgram& program, std::function<void()> init,
       std::function<proxy::Boolean()> cond, std::function<void()> body);
};

}  // namespace kush::compile::cpp::codegen