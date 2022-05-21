#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/aggregate_operator.h"

namespace kush::compile {

class AggregateTranslator : public OperatorTranslator {
 public:
  AggregateTranslator(
      const plan::AggregateOperator& agg, khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      execution::QueryState& state,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~AggregateTranslator() = default;

  void Produce(proxy::Pipeline& pipeline) override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::AggregateOperator& agg_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
  ExpressionTranslator expr_translator_;
  std::vector<std::unique_ptr<proxy::Aggregator>> aggregators_;
  std::unique_ptr<proxy::StructBuilder> agg_struct_;
  std::unique_ptr<proxy::Struct> value_;
  khir::Value empty_value_;
};

}  // namespace kush::compile