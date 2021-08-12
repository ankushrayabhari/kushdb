#pragma once

#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/compilation_cache.h"
#include "compile/program.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"
#include "plan/skinner_join_operator.h"

namespace kush::compile {

class RecompilingSkinnerJoinTranslator : public OperatorTranslator,
                                         public RecompilingJoinTranslator {
 public:
  RecompilingSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
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
      std::vector<proxy::Vector>& buffers,
      std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
      proxy::TupleIdxTable& tuple_idx_table,
      absl::flat_hash_set<int> evaluated_predicates,
      std::vector<absl::flat_hash_set<int>>& tables_per_predicate,
      std::vector<absl::btree_set<int>>& predicates_per_table,
      absl::flat_hash_set<int> available_tables, khir::Type idx_array_type,
      khir::Value idx_array, khir::Value progress_arr,
      khir::Value table_ctr_ptr, proxy::Int32 initial_budget,
      proxy::Bool resume_progress);

 private:
  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::vector<proxy::Vector> buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  absl::flat_hash_map<std::pair<int, int>, int> predicate_to_index_idx_;
  int child_idx_ = -1;
  CompilationCache cache_;
};

}  // namespace kush::compile