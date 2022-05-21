#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/aggregate_hash_table.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

GroupByAggregateTranslator::GroupByAggregateTranslator(
    const plan::GroupByAggregateOperator& group_by_agg,
    khir::ProgramBuilder& program, execution::PipelineBuilder& pipeline_builder,
    execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(group_by_agg, std::move(children)),
      group_by_agg_(group_by_agg),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program, *this) {}

void GroupByAggregateTranslator::Produce(proxy::Pipeline& output) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  std::vector<std::pair<catalog::Type, bool>> key_types;
  for (const auto& group_by : group_by_exprs) {
    key_types.emplace_back(group_by.get().Type(), group_by.get().Nullable());
  }

  std::vector<std::unique_ptr<proxy::Aggregator>> aggregators;
  for (const auto& agg : agg_exprs) {
    auto agg_type = agg.get().AggType();
    switch (agg_type) {
      case plan::AggregateType::AVG:
        aggregators.push_back(std::make_unique<proxy::AverageAggregator>(
            program_, expr_translator_, agg));
        break;
      case plan::AggregateType::SUM:
        aggregators.push_back(std::make_unique<proxy::SumAggregator>(
            program_, expr_translator_, agg));
        break;
      case plan::AggregateType::MIN:
      case plan::AggregateType::MAX:
        aggregators.push_back(std::make_unique<proxy::MinMaxAggregator>(
            program_, expr_translator_, agg,
            agg_type == plan::AggregateType::MIN));
        break;
      case plan::AggregateType::COUNT:
        aggregators.push_back(std::make_unique<proxy::CountAggregator>(
            program_, expr_translator_, agg));
        break;
    }
  }

  // Declare the hash table from group by keys -> struct list
  hash_table_ = std::make_unique<proxy::AggregateHashTable>(
      program_, state_, std::move(key_types), std::move(aggregators));

  // Populate hash table
  proxy::Pipeline input(program_, pipeline_builder_);
  input.Init([&]() { hash_table_->Init(); });
  input.Reset([&]() { hash_table_->Reset(); });
  input.Size([&]() { return hash_table_->NumTuples(); });
  this->Child().Produce(input);
  input.Build();

  // Loop over elements of HT and output row
  output.Body(input, [&](proxy::Int32 start, proxy::Int32 end) {
    hash_table_->ForEach(
        start, end, [&](std::vector<proxy::SQLValue> group_by_agg_values) {
          this->virtual_values_.SetValues(group_by_agg_values);

          // generate output variables
          this->values_.ResetValues();
          for (const auto& column : group_by_agg_.Schema().Columns()) {
            auto val = expr_translator_.Compute(column.Expr());
            this->values_.AddVariable(val);
          }

          if (auto parent = this->Parent()) {
            parent->get().Consume(*this);
          }
        });
  });
}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();

  std::vector<proxy::SQLValue> keys;
  for (const auto& group_by : group_by_exprs) {
    keys.push_back(expr_translator_.Compute(group_by.get()));
  }

  hash_table_->UpdateOrInsert(keys);
}

}  // namespace kush::compile