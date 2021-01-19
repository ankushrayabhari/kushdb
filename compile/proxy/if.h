#pragma once

#include <functional>
#include <utility>

#include "compile/program_builder.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::proxy {

template <typename T>
class If {
 public:
  If(ProgramBuilder<T>& program, const proxy::Boolean& cond,
     std::function<void()> then_fn)
      : program_(program) {
    b1 = program.CurrentBlock();
    b2 = program.GenerateBlock();
    auto& dest_block = program.GenerateBlock();

    // TODO: constant folding
    program.Branch(cond.Get(), b2.get(), dest_block);

    program.SetCurrentBlock(b2.get());
    then_fn();
    program.Branch(dest_block);

    program.SetCurrentBlock(dest_block);
  }

  If(ProgramBuilder<T>& program, const proxy::Boolean& cond,
     std::function<void()> then_fn, std::function<void()> else_fn)
      : program_(program) {
    b1 = program.GenerateBlock();
    b2 = program.GenerateBlock();
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

  If(const If&) = delete;
  If& operator=(const If&) = delete;
  If(If&&) = delete;
  If& operator=(If&&) = delete;

  Value& Phi(Value& v1, Value& v2) {
    auto& phi = program_.Phi(v1, b1.get(), v2, b2.get());
    program_.AddToPhi(phi, v1, b1.get());
    program_.AddToPhi(phi, v2, b2.get());
    return phi;
  }

 private:
  ProgramBuilder<T>& program_;
  std::reference_wrapper<typename ProgramBuilder<T>::BasicBlock> b1;
  std::reference_wrapper<typename ProgramBuilder<T>::BasicBlock> b2;
};

}  // namespace kush::compile::proxy