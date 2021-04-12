#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class ComparisonFunction {
 public:
  ComparisonFunction(
      ProgramBuilder<T>& program, StructBuilder<T> element,
      std::function<void(Struct<T>&, Struct<T>&, std::function<void(Bool<T>)>)>
          body);
  typename ProgramBuilder<T>::Function& Get();

 private:
  typename ProgramBuilder<T>::Function* func;
};

template <typename T>
class VoidFunction {
 public:
  VoidFunction(ProgramBuilder<T>& program, std::function<void()> body);
  typename ProgramBuilder<T>::Function& Get();

 private:
  typename ProgramBuilder<T>::Function* func;
};

}  // namespace kush::compile::proxy