#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/if.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class Loop {
 public:
  Loop(ProgramBuilder<T>& program,
       std::function<std::vector<std::unique_ptr<Value<T>>>()> init,
       std::function<std::unique_ptr<Bool<T>>(Loop<T>&)> cond,
       std::function<std::vector<std::unique_ptr<Value<T>>>(Loop<T>&)> body);

  template <typename S>
  std::unique_ptr<S> LoopVariable(int i) {
    return std::make_unique<S>(program_, loop_vars_[i].get());
  }

 private:
  ProgramBuilder<T>& program_;
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      loop_vars_;
};

}  // namespace kush::compile::proxy