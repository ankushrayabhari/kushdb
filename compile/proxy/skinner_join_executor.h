#pragma once

#include <functional>
#include <vector>

#include "absl/types/span.h"

#include "compile/proxy/int.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class TableFunction {
 public:
  TableFunction(khir::ProgramBuilder& program,
                std::function<proxy::Int32(proxy::Int32&, proxy::Int8&)> body);

  khir::FunctionRef Get();

 private:
  std::optional<khir::FunctionRef> func_;
};

class SkinnerJoinExecutor {
 public:
  SkinnerJoinExecutor(khir::ProgramBuilder& program);

  void ExecutePermutableJoin(absl::Span<const khir::Value> args);
  void ExecuteRecompilingJoin(RecompilingJoinTranslator* obj);

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile::proxy