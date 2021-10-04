#include "compile/proxy/if.h"

#include <functional>
#include <utility>

#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn) {
  auto first_block = program.GenerateBlock();
  auto dest_block = program.GenerateBlock();
  program.Branch(cond.Get(), first_block, dest_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn) {
  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();
  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(second_block);
  else_fn();

  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

std::vector<khir::Value> Ternary(
    khir::ProgramBuilder& program, const Bool& cond,
    std::function<std::vector<khir::Value>()> then_fn,
    std::function<std::vector<khir::Value>()> else_fn) {
  std::vector<khir::Type> phi_types;
  std::vector<khir::Value> then_branch_phi_members;
  std::vector<khir::Value> else_branch_phi_members;
  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  const auto then_branch_values = then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    for (auto v : then_branch_values) {
      then_branch_phi_members.push_back(program.PhiMember(v));
      phi_types.push_back(program.TypeOf(v));
    }
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(second_block);
  const auto else_branch_values = else_fn();

  if (!program.IsTerminated(program.CurrentBlock())) {
    for (auto v : else_branch_values) {
      else_branch_phi_members.push_back(program.PhiMember(v));
    }
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);

  if (phi_types.size() > 0) {
    std::vector<khir::Value> phi_values;
    for (int i = 0; i < phi_types.size(); i++) {
      phi_values.push_back(program.Phi(phi_types[i]));
    }
    for (int i = 0; i < then_branch_phi_members.size(); i++) {
      program.UpdatePhiMember(phi_values[i], then_branch_phi_members[i]);
    }
    for (int i = 0; i < else_branch_phi_members.size(); i++) {
      program.UpdatePhiMember(phi_values[i], else_branch_phi_members[i]);
    }
    return phi_values;
  } else {
    return {};
  }
}

}  // namespace kush::compile::proxy