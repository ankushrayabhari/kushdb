#include "compile/proxy/loop.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
Loop<T>::Loop(ProgramBuilder<T>& program, std::function<void(Loop&)> init,
              std::function<Bool<T>(Loop&)> cond,
              std::function<typename Loop<T>::Continuation(Loop&)> body)
    : program_(program) {
  header_ = &program.GenerateBlock();
  // Initialize all loop variables value
  init(*this);

  // now that all loop varaibles are knowns, add to phi nodes
  for (int i = 0; i < phi_nodes_.size(); i++) {
    program_.AddToPhi(phi_nodes_[i], phi_nodes_initial_values_[i],
                      program_.CurrentBlock());
  }

  // move to header
  program_.Branch(*header_);
  program_.SetCurrentBlock(*header_);

  // check the condition
  auto c = cond(*this);

  // jump to either body or end
  auto& body_block = program_.GenerateBlock();
  end_ = &program_.GenerateBlock();
  program_.Branch(c.Get(), body_block, *end_);

  // run body
  program_.SetCurrentBlock(body_block);
  body(*this);

  // set block to end
  program_.SetCurrentBlock(*end_);
}

template <typename T>
void Loop<T>::AddLoopVariable(const proxy::Value<T>& v) {
  // switch to header
  auto& current_block = program_.CurrentBlock();
  program_.SetCurrentBlock(*header_);

  // add a phi node
  phi_nodes_.push_back(program_.Phi(program_.TypeOf(v.Get())));

  // delay setting the initial value until all others have been computed
  phi_nodes_initial_values_.push_back(v.Get());

  // switch back to original block
  program_.SetCurrentBlock(current_block);
}

template <typename T>
void Loop<T>::Break() {
  program_.Branch(*end_);
}

INSTANTIATE_ON_IR(Loop);

}  // namespace kush::compile::proxy