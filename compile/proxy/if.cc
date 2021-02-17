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
    : program_(program),
      b1(program.CurrentBlock()),
      b2(program.GenerateBlock()) {
  auto& dest_block = program.GenerateBlock();

  program.Branch(cond.Get(), b2.get(), dest_block);

  program.SetCurrentBlock(b2.get());
  then_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

template <typename T>
If<T>::If(ProgramBuilder<T>& program, const Bool<T>& cond,
          std::function<void()> then_fn, std::function<void()> else_fn)
    : program_(program),
      b1(program.GenerateBlock()),
      b2(program.GenerateBlock()) {
  typename ProgramBuilder<T>::BasicBlock* dest_block = nullptr;

  program.Branch(cond.Get(), b1.get(), b2.get());

  program.SetCurrentBlock(b1.get());
  then_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    if (dest_block == nullptr) {
      dest_block = &program.GenerateBlock();
    }

    program.Branch(*dest_block);
  }

  program.SetCurrentBlock(b2.get());
  else_fn();
  if (!program_.IsTerminated(program_.CurrentBlock())) {
    if (dest_block == nullptr) {
      dest_block = &program.GenerateBlock();
    }

    program.Branch(*dest_block);
  }

  if (dest_block != nullptr) {
    program.SetCurrentBlock(*dest_block);
  }
}

INSTANTIATE_ON_IR(If);

}  // namespace kush::compile::proxy