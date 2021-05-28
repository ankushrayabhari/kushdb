#pragma once

#include <functional>
#include <vector>

#include "absl/types/span.h"

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

class TableFunction {
 public:
  TableFunction(khir::KHIRProgramBuilder& program,
                std::function<proxy::Int32(proxy::Int32&, proxy::Int8&)> body);

  khir::FunctionRef Get();

 private:
  std::optional<khir::FunctionRef> func_;
};

class SkinnerJoinExecutor {
 public:
  SkinnerJoinExecutor(khir::KHIRProgramBuilder& program);

  void Execute(absl::Span<const khir::Value> args);

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);

 private:
  khir::KHIRProgramBuilder& program_;
};

}  // namespace kush::compile::proxy