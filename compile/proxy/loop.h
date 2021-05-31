#pragma once

#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class Loop {
 public:
  class Continuation {
    friend class Loop;

   private:
    Continuation() = default;
  };

  Loop(khir::ProgramBuilder& program, std::function<void(Loop&)> init,
       std::function<Bool(Loop&)> cond,
       std::function<Continuation(Loop&)> body);

  void AddLoopVariable(const proxy::Value& v);

  template <typename S>
  S GetLoopVariable(int i) {
    return S(program_, phi_nodes_[i]);
  }

  Continuation Continue() {
    program_.Branch(header_);
    return Continuation();
  }

  template <typename... Args>
  Continuation Continue(const proxy::Value& first, Args const&... rest) {
    int i = phi_nodes_.size() - 1 - sizeof...(rest);
    auto phi_member = program_.PhiMember(first.Get());
    program_.UpdatePhiMember(phi_nodes_[i], phi_member);
    if (sizeof...(rest)) {
      return Continue(rest...);
    } else {
      program_.Branch(header_);
      return Continuation();
    }
  }

 private:
  khir::ProgramBuilder& program_;
  std::vector<khir::Value> phi_nodes_;
  std::vector<khir::Value> phi_nodes_initial_values_;
  khir::BasicBlockRef header_;
  khir::BasicBlockRef end_;
};

}  // namespace kush::compile::proxy