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
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/recompiling_join_translator.h"
#include "execution/pipeline.h"
#include "khir/compilation_cache.h"
#include "khir/program.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_join_operator.h"

namespace kush::compile {

class RecompilingSkinnerJoinTranslator : public OperatorTranslator,
                                         public RecompilingJoinTranslator {
 public:
  RecompilingSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~RecompilingSkinnerJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;
  ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order,
                                 void** materialized_buffers,
                                 void** materialized_indexes,
                                 void* tuple_idx_table) override;
  proxy::Int32 GenerateChildLoops(
      int curr, const std::vector<int>& order, khir::ProgramBuilder& program,
      ExpressionTranslator& expr_translator,
      std::vector<std::unique_ptr<proxy::MaterializedBuffer>>&
          materialized_buffers,
      std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
      const std::vector<proxy::Int32>& cardinalities,
      proxy::TupleIdxTable& tuple_idx_table,
      absl::flat_hash_set<int>& evaluated_conditions,
      absl::flat_hash_set<int>& available_tables, khir::Value idx_array,
      khir::Value offset_array, khir::Value progress_arr,
      khir::Value table_ctr_ptr, khir::Value num_result_tuples_ptr,
      proxy::Int32 initial_budget, proxy::Bool resume_progress,
      std::vector<proxy::ColumnIndexBucketArray>& bucket_lists,
      const std::vector<khir::Value>& results, const int result_max_size);

 private:
  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  ExpressionTranslator expr_translator_;
  proxy::Vector* buffer_;
  std::vector<std::unique_ptr<proxy::MaterializedBuffer>> materialized_buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  absl::flat_hash_map<std::pair<int, int>, int> column_to_index_idx_;
  std::vector<absl::flat_hash_set<int>> tables_per_condition_;
  std::vector<absl::btree_set<int>> conditions_per_table_;
  absl::flat_hash_set<std::pair<int, int>> table_connections_;
  int child_idx_ = -1;
  khir::CompilationCache cache_;
};

}  // namespace kush::compile