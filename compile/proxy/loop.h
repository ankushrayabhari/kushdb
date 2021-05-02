#pragma once

#include <functional>
#include <iostream>
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
  class Continuation {
    friend class Loop;

   private:
    Continuation() = default;
  };

  Loop(ProgramBuilder<T>& program, std::function<void(Loop&)> init,
       std::function<Bool<T>(Loop&)> cond,
       std::function<Continuation(Loop&)> body);

  void AddLoopVariable(const proxy::Value<T>& v);

  void Break();

  template <typename S>
  S GetLoopVariable(int i) {
    return S(program_, phi_nodes_[i]);
  }

  Continuation Continue() {
    program_.Branch(header_);
    return Continuation();
  }

  template <typename... Args>
  Continuation Continue(const proxy::Value<T>& first, Args const&... rest) {
    int i = phi_nodes_.size() - 1 - sizeof...(rest);
    program_.AddToPhi(phi_nodes_[i], first.Get(), program_.CurrentBlock());
    if (sizeof...(rest)) {
      return Continue(rest...);
    } else {
      program_.Branch(header_);
      return Continuation();
    }
  }

 private:
  ProgramBuilder<T>& program_;
  std::vector<typename ProgramBuilder<T>::PhiValue> phi_nodes_;
  std::vector<typename ProgramBuilder<T>::Value> phi_nodes_initial_values_;
  typename ProgramBuilder<T>::BasicBlock header_;
  typename ProgramBuilder<T>::BasicBlock end_;
};

}  // namespace kush::compile::proxy