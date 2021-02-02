#include "compile/proxy/loop.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/if.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
Loop<T>::Loop(
    ProgramBuilder<T>& program,
    std::function<std::vector<std::unique_ptr<Value<T>>>()> init,
    std::function<std::unique_ptr<Bool<T>>(Loop<T>&)> cond,
    std::function<std::vector<std::unique_ptr<Value<T>>>(Loop<T>&)> body)
    : program_(program) {
  auto initial_values = init();
  auto& init_block = program.CurrentBlock();

  // create the loop variable
  loop_vars_.reserve(initial_values.size());
  for (auto& value : initial_values) {
    auto& phi = program.Phi();
    program.AddToPhi(phi, value->Get(), init_block);
    loop_vars_.push_back(phi);
  }

  // check the condition
  auto cond_var = cond(*this);
  If<T>(program, *cond_var, [&]() {
    auto updated_loop_vars = body(*this);
    auto& body_block = program.CurrentBlock();
    for (int i = 0; i < updated_loop_vars.size(); i++) {
      auto& phi = loop_vars_[i].get();
      program.AddToPhi(phi, updated_loop_vars[i]->Get(), body_block);
    }
  });
}

INSTANTIATE_ON_IR(Loop);

}  // namespace kush::compile::proxy