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

template <typename T>
T TernaryImpl(khir::ProgramBuilder& program, const Bool& cond,
              std::function<T()> then_fn, std::function<T()> else_fn) {
  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  const auto then_branch_value = then_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  auto then_branch_phi_member = program.PhiMember(then_branch_value.Get());
  khir::Type phi_type = program.TypeOf(then_branch_value.Get());
  program.Branch(dest_block);

  program.SetCurrentBlock(second_block);
  const auto else_branch_value = else_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  auto else_branch_phi_member = program.PhiMember(else_branch_value.Get());
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
  auto phi_value = program.Phi(phi_type);
  program.UpdatePhiMember(phi_value, then_branch_phi_member);
  program.UpdatePhiMember(phi_value, else_branch_phi_member);
  return T(program, phi_value);
}

proxy::Bool Ternary(khir::ProgramBuilder& program, const Bool& cond,
                    std::function<proxy::Bool()> then_fn,
                    std::function<proxy::Bool()> else_fn) {
  return TernaryImpl<proxy::Bool>(program, cond, then_fn, else_fn);
}

proxy::Int8 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                    std::function<proxy::Int8()> then_fn,
                    std::function<proxy::Int8()> else_fn) {
  return TernaryImpl<proxy::Int8>(program, cond, then_fn, else_fn);
}

proxy::Int16 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int16()> then_fn,
                     std::function<proxy::Int16()> else_fn) {
  return TernaryImpl<proxy::Int16>(program, cond, then_fn, else_fn);
}

proxy::Int32 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int32()> then_fn,
                     std::function<proxy::Int32()> else_fn) {
  return TernaryImpl<proxy::Int32>(program, cond, then_fn, else_fn);
}

proxy::Int64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int64()> then_fn,
                     std::function<proxy::Int64()> else_fn) {
  return TernaryImpl<proxy::Int64>(program, cond, then_fn, else_fn);
}

proxy::Float64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                       std::function<proxy::Float64()> then_fn,
                       std::function<proxy::Float64()> else_fn) {
  return TernaryImpl<proxy::Float64>(program, cond, then_fn, else_fn);
}

proxy::String Ternary(khir::ProgramBuilder& program, const Bool& cond,
                      std::function<proxy::String()> then_fn,
                      std::function<proxy::String()> else_fn) {
  return TernaryImpl<proxy::String>(program, cond, then_fn, else_fn);
}

}  // namespace kush::compile::proxy