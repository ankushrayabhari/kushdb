#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"

#include "compile/proxy/value/ir_value.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class SkinnerJoinExecutor {
 public:
  static void ExecutePermutableJoin(
      khir::ProgramBuilder& program, int32_t num_tables, int32_t num_preds,
      absl::flat_hash_map<std::pair<int, int>, int>* pred_table_to_flag,
      absl::flat_hash_set<std::pair<int, int>>* table_connections,
      const std::vector<int>* prefix_order, khir::Value join_handler_fn_arr,
      khir::Value valid_tuple_handler, int32_t num_flags, khir::Value flag_arr,
      khir::Value progress_arr, khir::Value table_ctr, khir::Value idx_arr,
      khir::Value num_result_tuples, khir::Value offset_arr);
  static void ExecuteRecompilingJoin(
      khir::ProgramBuilder& program, int32_t num_tables,
      khir::Value cardinality_arr,
      absl::flat_hash_set<std::pair<int, int>>* table_connections,
      const std::vector<int>* prefix_order, RecompilingJoinTranslator* obj,
      khir::Value materialized_buffers, khir::Value materialized_indexes,
      khir::Value tuple_idx_table);

  static void ForwardDeclare(khir::ProgramBuilder& program);
};

}  // namespace kush::compile::proxy