#pragma once

#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_index.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/vector.h"
#include "compile/translators/compilation_cache.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/recompiling_join_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_join_operator.h"

namespace kush::compile {

class PermutableCache {
 public:
  PermutableCache(int num_tables);

  int8_t* AllocateFlags(int num_flags);
  void* TableHandlers();

  bool IsCompiled() const;
  void Compile(std::unique_ptr<khir::Program> program,
               std::vector<std::string> function_names);

  void SetFlagInfo(
      int total_preds,
      absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag);

  void* Order(const std::vector<int>& order);

  void SetValidTupleHandler(void* table_handler);

 private:
  int num_tables_;
  int total_preds_;
  void** table_handlers_;
  int num_flags_;
  int8_t* flags_;

  void* valid_tuple_handler_;

  std::vector<void*> table_functions_;
  absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag_;

  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<khir::Backend> backend_;
};

class HybridSkinnerJoinTranslator : public OperatorTranslator,
                                    public RecompilingJoinTranslator {
 public:
  HybridSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      execution::QueryState& state,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HybridSkinnerJoinTranslator() = default;
  void Produce(proxy::Pipeline&) override;
  void Consume(OperatorTranslator& src) override;
  ExecuteJoinFn CompileJoinOrder(
      const std::vector<int>& order, void** materialized_buffers_raw,
      void** materialized_indexes_raw, void* tuple_idx_table_ptr_raw,
      int32_t* progress_arr_raw, int32_t* table_ctr_raw, int32_t* idx_arr_raw,
      int32_t* offset_arr_raw,
      std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_raw)
      override;

 private:
  bool ShouldExecute(int pred, int table_idx,
                     const absl::flat_hash_set<int>& available_tables);
  void CompileFullJoinOrder(
      CacheEntry& entry, const std::vector<int>& order,
      void** materialized_buffers_raw, void** materialized_indexes_raw,
      void* tuple_idx_table_ptr_raw, int32_t* progress_arr_raw,
      int32_t* table_ctr_raw, int32_t* idx_arr_raw, int32_t* offset_arr_raw,
      std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_raw);
  void CompilePermutable(
      PermutableCache& entry, void** materialized_buffers_raw,
      void** materialized_indexes_raw, void* tuple_idx_table_ptr_raw,
      int32_t* progress_arr_raw, int32_t* table_ctr_raw, int32_t* idx_arr_raw,
      int32_t* offset_arr_raw,
      std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_raw);

  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder* program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
  std::unique_ptr<ExpressionTranslator> expr_translator_;
  proxy::Vector* buffer_;
  std::vector<std::unique_ptr<proxy::MaterializedBuffer>> materialized_buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  absl::flat_hash_map<std::pair<int, int>, int> column_to_index_idx_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  std::vector<
      std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>>
      columns_per_predicate_;
  std::vector<absl::flat_hash_set<int>> tables_per_condition_;
  std::vector<absl::btree_set<int>> conditions_per_table_;
  absl::flat_hash_set<std::pair<int, int>> table_connections_;
  int child_idx_ = -1;
  CompilationCache cache_;
  PermutableCache permutable_cache_;
};

}  // namespace kush::compile