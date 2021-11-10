#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"

#include "compile/proxy/value/ir_value.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class TableFunction {
 public:
  TableFunction(khir::ProgramBuilder& program,
                std::function<Int32(Int32&, Bool&)> body);

  khir::FunctionRef Get();

 private:
  std::optional<khir::FunctionRef> func_;
};

class SkinnerJoinExecutor {
 public:
  SkinnerJoinExecutor(khir::ProgramBuilder& program);

  void ExecutePermutableJoin(absl::Span<const khir::Value> args);
  void ExecuteRecompilingJoin(
      int32_t num_tables, khir::Value cardinality_arr,
      absl::flat_hash_set<std::pair<int, int>>* table_connections,
      RecompilingJoinTranslator* obj, khir::Value materialized_buffers,
      khir::Value materialized_indexes, khir::Value tuple_idx_table);

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile::proxy