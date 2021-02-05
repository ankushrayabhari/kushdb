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
  IndexLoop(ProgramBuilder<T>& program, std::function<UInt32<T>()> init,
            std::function<Bool<T>(UInt32<T>&)> cond,
            std::function<UInt32<T>(UInt32<T>&)> body);

 private:
  ProgramBuilder<T>& program_;
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      loop_vars_;
};

}  // namespace kush::compile::proxy