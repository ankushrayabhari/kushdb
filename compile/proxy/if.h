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

  template <typename U>
  typename ProgramBuilder<T>::Value& Phi(U& v1, U& v2) {
    if (b1 == nullptr || b2 == nullptr) {
      throw std::runtime_error("Don't call phi on a single predecessor block.");
    }
    auto& phi = program_.Phi(program_.TypeOf(v1.Get()));
    program_.AddToPhi(phi, v1.Get(), *b1);
    program_.AddToPhi(phi, v2.Get(), *b2);
    return phi;
  }

  template <>
  typename ProgramBuilder<T>::Value& Phi(
      typename ProgramBuilder<T>::Value& v1,
      typename ProgramBuilder<T>::Value& v2) {
    if (b1 == nullptr || b2 == nullptr) {
      throw std::runtime_error("Don't call phi on a single predecessor block.");
    }
    auto& phi = program_.Phi(program_.TypeOf(v1));
    program_.AddToPhi(phi, v1, *b1);
    program_.AddToPhi(phi, v2, *b2);
    return phi;
  }

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::BasicBlock* b1;
  typename ProgramBuilder<T>::BasicBlock* b2;
};

}  // namespace kush::compile::proxy