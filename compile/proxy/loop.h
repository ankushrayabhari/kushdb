#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/boolean.h"
#include "compile/proxy/if.h"

namespace kush::compile::proxy {

template <typename T>
class Loop {
 public:
  Loop(ProgramBuilder<T>& program,
       std::function<std::vector<std::reference_wrapper<proxy::Value>>>() >
           init,
       std::function<proxy::Boolean(Loop&)> cond,
       std::function<std::vector<std::reference_wrapper<proxy::Value>>>(Loop&) >
           body) {
    auto initial_values = init();
    auto& init_block = program.CurrentBlock();

    // create the loop variable
    loop_vars_.reserve(initial_values.size());
    for (auto& value : initial_values) {
      auto& phi = program_.Phi();
      program.AddToPhi(phi, value.get(), init_block);
      loop_vars_.push_back(phi);
    }

    // check the condition
    auto cond_var = cond(*this);
    If check(program, cond_var.get(), [&]() {
      auto updated_loop_vars = body(*this);
      auto& body_block = program.CurrentBlock();
      for (int i = 0; i < updated_loop_vars.size(); i++) {
        auto& phi = LoopVariable(i);
        program.AddToPhi(phi, updated_loop_vars[i].get(), body_block);
      }
    });
  }

  Value& LoopVariable(int i);

 private:
  std::vector<std::reference_wrapper<Value>> loop_vars_;
};

}  // namespace kush::compile::proxy