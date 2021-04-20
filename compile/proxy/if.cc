#include "compile/proxy/if.h"

#include <functional>
#include <utility>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"

namespace kush::compile::proxy {

template <typename T>
If<T>::If(ProgramBuilder<T>& program, const Bool<T>& cond,
          std::function<void()> then_fn)
    : program_(program), b1(&program.CurrentBlock()), b2(nullptr) {
  auto& dest_block = program.GenerateBlock();

  auto& then_block = program.GenerateBlock();

  program.Branch(cond.Get(), then_block, dest_block);

  program.SetCurrentBlock(then_block);
  then_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    b2 = &program_.CurrentBlock();
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

template <typename T>
If<T>::If(ProgramBuilder<T>& program, const Bool<T>& cond,
          std::function<void()> then_fn, std::function<void()> else_fn)
    : program_(program), b1(nullptr), b2(nullptr) {
  typename ProgramBuilder<T>::BasicBlock* dest_block = nullptr;

  auto& first_block = program.GenerateBlock();
  auto& second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    if (dest_block == nullptr) {
      dest_block = &program.GenerateBlock();
    }

    b1 = &program_.CurrentBlock();
    program.Branch(*dest_block);
  }

  program.SetCurrentBlock(second_block);
  else_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    if (dest_block == nullptr) {
      dest_block = &program.GenerateBlock();
    }

    b2 = &program_.CurrentBlock();
    program.Branch(*dest_block);
  }

  if (dest_block != nullptr) {
    program.SetCurrentBlock(*dest_block);
  }
}

INSTANTIATE_ON_IR(If);

}  // namespace kush::compile::proxy