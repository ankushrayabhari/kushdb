#include "compile/proxy/loop.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/bool.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

Loop::Loop(khir::ProgramBuilder& program, std::function<void(Loop&)> init,
           std::function<Bool(Loop&)> cond,
           std::function<Loop::Continuation(Loop&)> body)
    : program_(program),
      header_(program.GenerateBlock()),
      end_(program_.GenerateBlock()) {
  // Initialize all loop variables value
  init(*this);
  std::vector<khir::Value> initial_phi_members_;

  // now that all loop varaibles are known, add to phi nodes
  for (int i = 0; i < phi_nodes_initial_values_.size(); i++) {
    initial_phi_members_.push_back(
        program_.PhiMember(phi_nodes_initial_values_[i]));
  }

  // move to header
  program_.Branch(header_);
  program_.SetCurrentBlock(header_);

  // create phi nodes and update initial phi members to point to phi node
  for (int i = 0; i < phi_nodes_initial_values_.size(); i++) {
    auto phi = program_.Phi(program_.TypeOf(phi_nodes_initial_values_[i]));
    program_.UpdatePhiMember(phi, initial_phi_members_[i]);
    phi_nodes_.push_back(phi);
  }

  // check the condition
  auto c = cond(*this);

  // jump to either body or end
  auto body_block = program_.GenerateBlock();
  program_.Branch(c.Get(), body_block, end_);

  // run body
  program_.SetCurrentBlock(body_block);
  body(*this);

  // set block to end
  program_.SetCurrentBlock(end_);
}

void Loop::AddLoopVariable(const proxy::Value& v) {
  // delay setting the initial value until all others have been computed
  phi_nodes_initial_values_.push_back(v.Get());
}

}  // namespace kush::compile::proxy