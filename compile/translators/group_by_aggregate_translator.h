#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/operator/group_by_aggregate_operator.h"

namespace kush::compile {

class GroupByAggregateTranslator : public OperatorTranslator {
 public:
  GroupByAggregateTranslator(
      const plan::GroupByAggregateOperator& group_by_agg,
      khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~GroupByAggregateTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::GroupByAggregateOperator& group_by_agg_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::HashTable> buffer_;
  std::vector<std::unique_ptr<proxy::Aggregator>> aggregators_;
  khir::Value found_;
};

}  // namespace kush::compile