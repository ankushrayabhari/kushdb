#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/if.h"

namespace kush::compile::proxy {

template <typename T>
class Loop {
 public:
  Loop(ProgramBuilder<T>& program,
       std::function<std::vector<
           std::reference_wrapper<typename ProgramBuilder<T>::Value>>()>
           init,
       std::function<Bool<T>(Loop<T>&)> cond,
       std::function<std::vector<
           std::reference_wrapper<typename ProgramBuilder<T>::Value>>(Loop<T>&)>
           body);
  typename ProgramBuilder<T>::Value& LoopVariable(int i);

 private:
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      loop_vars_;
};

}  // namespace kush::compile::proxy