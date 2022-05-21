#pragma once

#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_index.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/proxy/struct.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_join_operator.h"

namespace kush::compile {

class PermutableSkinnerJoinTranslator : public OperatorTranslator {
 public:
  PermutableSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      execution::QueryState& state,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~PermutableSkinnerJoinTranslator() = default;
  void Produce(proxy::Pipeline&) override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
  ExpressionTranslator expr_translator_;
  proxy::Vector* buffer_;
  std::vector<std::unique_ptr<proxy::MaterializedBuffer>> materialized_buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  absl::flat_hash_map<std::pair<int, int>, int> column_to_index_idx_;
  absl::flat_hash_map<std::pair<int, int>, int> pred_table_to_flag_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  absl::flat_hash_set<std::pair<int, int>> table_connections_;
  int child_idx_ = -1;
};

}  // namespace kush::compile