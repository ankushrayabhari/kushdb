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
class Loop {
 public:
  Loop(
      ProgramBuilder<T>& program, std::function<void(Loop&)> init,
      std::function<Bool<T>(Loop&)> cond,
      std::function<std::vector<std::unique_ptr<proxy::Value<T>>>(Loop&)> body);

  void AddLoopVariable(const proxy::Value<T>& v);

  void Break();

  void Continue(std::vector<std::reference_wrapper<proxy::Value<T>>> loop_vars);

  template <typename S>
  S GetLoopVariable(int i) {
    return S(program_, phi_nodes_[i].get());
  }

 private:
  ProgramBuilder<T>& program_;
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::PhiValue>>
      phi_nodes_;
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      phi_nodes_initial_values_;
  typename ProgramBuilder<T>::BasicBlock* header_;
  typename ProgramBuilder<T>::BasicBlock* end_;
};

}  // namespace kush::compile::proxy