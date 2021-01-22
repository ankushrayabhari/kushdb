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

  // TODO: constant folding
  program.Branch(cond.Get(), b2.get(), dest_block);

  program.SetCurrentBlock(b2.get());
  then_fn();
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
}

template <typename T>
If<T>::If(ProgramBuilder<T>& program, const Bool<T>& cond,
          std::function<void()> then_fn, std::function<void()> else_fn)
    : program_(program),
      b1(program.GenerateBlock()),
      b2(program.GenerateBlock()) {
  auto& dest_block = program.GenerateBlock();

  // TODO: constant folding
  program.Branch(cond.Get(), b1.get(), b2.get());

  program.SetCurrentBlock(b1.get());
  then_fn();
  program.Branch(dest_block);

  program.SetCurrentBlock(b2.get());
  else_fn();
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
}

INSTANTIATE_ON_IR(If);

}  // namespace kush::compile::proxy