#pragma once

#include <functional>
#include <utility>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"

namespace kush::compile::proxy {

template <typename T>
class If {
 public:
  If(ProgramBuilder<T>& program, const Bool<T>& cond,
     std::function<void()> then_fn);
  If(ProgramBuilder<T>& program, const Bool<T>& cond,
     std::function<void()> then_fn, std::function<void()> else_fn);

  If(const If<T>&) = delete;
  If<T>& operator=(const If<T>&) = delete;
  If(If<T>&&) = delete;
  If<T>& operator=(If<T>&&) = delete;

 private:
  ProgramBuilder<T>& program_;
  std::reference_wrapper<typename ProgramBuilder<T>::BasicBlock> b1;
  std::reference_wrapper<typename ProgramBuilder<T>::BasicBlock> b2;
};

}  // namespace kush::compile::proxy