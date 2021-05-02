#pragma once

#include <functional>
#include <vector>

#include "absl/types/span.h"

#include "compile/program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

template <typename T>
class TableFunction {
 public:
  TableFunction(
      ProgramBuilder<T>& program,
      std::function<proxy::Int32<T>(proxy::Int32<T>&, proxy::Int8<T>&)> body);

  typename ProgramBuilder<T>::Function Get();

 private:
  std::optional<typename ProgramBuilder<T>::Function> func_;
};

template <typename T>
class SkinnerJoinExecutor {
 public:
  SkinnerJoinExecutor(ProgramBuilder<T>& program);

  void Execute(absl::Span<const typename ProgramBuilder<T>::Value> args);

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
};

}  // namespace kush::compile::proxy