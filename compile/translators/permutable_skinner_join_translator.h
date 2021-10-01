#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/column_index.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/skinner_join_operator.h"

namespace kush::compile {

class PermutableSkinnerJoinTranslator : public OperatorTranslator {
 public:
  PermutableSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~PermutableSkinnerJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  ExpressionTranslator expr_translator_;
  std::vector<proxy::Vector> buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  absl::flat_hash_map<std::pair<int, int>, int> predicate_to_index_idx_;
  int child_idx_ = -1;
  std::vector<std::unique_ptr<execution::Pipeline>> child_pipelines;
};

}  // namespace kush::compile