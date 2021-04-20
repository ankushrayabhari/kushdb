#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class IndexLoop {
 public:
  IndexLoop(
      ProgramBuilder<T>& program, std::function<Int32<T>()> init,
      std::function<Bool<T>(Int32<T>&)> cond,
      std::function<Int32<T>(Int32<T>&, std::function<void(Int32<T>&)>)> body);
};

}  // namespace kush::compile::proxy