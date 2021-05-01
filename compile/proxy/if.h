#pragma once

#include <functional>
#include <utility>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"

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

  proxy::Bool<T> Phi(const proxy::Bool<T>& v1, const proxy::Bool<T>& v2) {
    return PhiImpl<proxy::Bool<T>>(v1, v2);
  }

  proxy::Float64<T> Phi(const proxy::Float64<T>& v1,
                        const proxy::Float64<T>& v2) {
    return PhiImpl<proxy::Float64<T>>(v1, v2);
  }

  proxy::Int8<T> Phi(const proxy::Int8<T>& v1, const proxy::Int8<T>& v2) {
    return PhiImpl<proxy::Int8<T>>(v1, v2);
  }

  proxy::Int16<T> Phi(const proxy::Int16<T>& v1, const proxy::Int16<T>& v2) {
    return PhiImpl<proxy::Int16<T>>(v1, v2);
  }

  proxy::Int32<T> Phi(const proxy::Int32<T>& v1, const proxy::Int32<T>& v2) {
    return PhiImpl<proxy::Int32<T>>(v1, v2);
  }

  proxy::Int64<T> Phi(const proxy::Int64<T>& v1, const proxy::Int64<T>& v2) {
    return PhiImpl<proxy::Int64<T>>(v1, v2);
  }

  proxy::String<T> Phi(const proxy::String<T>& v1, const proxy::String<T>& v2) {
    return PhiImpl<proxy::String<T>>(v1, v2);
  }

 private:
  template <typename S>
  S PhiImpl(const S& v1, const S& v2) {
    if (b1 == nullptr || b2 == nullptr) {
      throw std::runtime_error("Don't call phi on a single predecessor block.");
    }
    auto& phi = program_.Phi(program_.TypeOf(v1.Get()));
    program_.AddToPhi(phi, v1.Get(), *b1);
    program_.AddToPhi(phi, v2.Get(), *b2);
    return S(program_, phi);
  }

  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::BasicBlock* b1;
  typename ProgramBuilder<T>::BasicBlock* b2;
};

template <typename T, typename S>
S Ternary(ProgramBuilder<T>& program, const proxy::Bool<T>& cond,
          std::function<S()> left, std::function<S()> right) {
  std::unique_ptr<S> l, r;
  If<T> check(
      program, cond, [&]() { l = left().ToPointer(); },
      [&]() { r = right().ToPointer(); });
  return check.Phi(*l, *r);
}

}  // namespace kush::compile::proxy