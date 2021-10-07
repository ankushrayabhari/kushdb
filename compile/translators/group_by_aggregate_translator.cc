#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

GroupByAggregateTranslator::GroupByAggregateTranslator(
    const plan::GroupByAggregateOperator& group_by_agg,
    khir::ProgramBuilder& program, execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(group_by_agg, std::move(children)),
      group_by_agg_(group_by_agg),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program, *this) {}

void GroupByAggregateTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto& pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // Struct for hash table
  proxy::StructBuilder packed(program_);
  // Include every group by column inside the struct
  for (const auto& group_by : group_by_exprs) {
    packed.Add(group_by.get().Type(), group_by.get().Nullable());
  }
  for (const auto& agg : agg_exprs) {
    auto agg_type = agg.get().AggType();
    switch (agg_type) {
      case plan::AggregateType::AVG:
        aggregators_.push_back(std::make_unique<proxy::AverageAggregator>(
            program_, expr_translator_, agg));
        break;
      case plan::AggregateType::SUM:
        aggregators_.push_back(std::make_unique<proxy::SumAggregator>(
            program_, expr_translator_, agg));
        break;
      case plan::AggregateType::MIN:
      case plan::AggregateType::MAX:
        aggregators_.push_back(std::make_unique<proxy::MinMaxAggregator>(
            program_, expr_translator_, agg,
            agg_type == plan::AggregateType::MIN));
        break;
      case plan::AggregateType::COUNT:
        aggregators_.push_back(std::make_unique<proxy::CountAggregator>(
            program_, expr_translator_, agg));
        break;
    }
    aggregators_.back()->AddFields(packed);
  }
  packed.Build();

  // Declare the hash table from group by keys -> struct list
  buffer_ = std::make_unique<proxy::HashTable>(program_, packed);

  // Declare the found variable
  found_ = program_.Alloca(program_.I8Type());

  // Populate hash table
  this->Child().Produce();

  // Finish pipeline
  program_.Return();
  auto child_pipeline = pipeline_builder_.FinishPipeline();
  pipeline_builder_.GetCurrentPipeline().AddPredecessor(
      std::move(child_pipeline));

  // Loop over elements of HT and output row
  program_.SetCurrentBlock(output_pipeline_func);
  buffer_->ForEach([&](proxy::Struct& packed) {
    auto values = packed.Unpack();
    for (int i = 0; i < group_by_exprs.size(); i++) {
      this->virtual_values_.AddVariable(std::move(values[i]));
    }
    for (auto& aggregator : aggregators_) {
      this->virtual_values_.AddVariable(aggregator->Get(values));
    }

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
  buffer_->Reset();
}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // keys = all group by exprs
  std::vector<proxy::SQLValue> keys;
  for (const auto& group_by : group_by_exprs) {
    keys.push_back(expr_translator_.Compute(group_by.get()));
  }
  program_.StoreI8(found_, proxy::Int8(program_, 0).Get());

  // Loop over bucket if exists
  auto bucket = buffer_->Get(util::ReferenceVector(keys));
  auto size = bucket.Size();

  proxy::Loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < size;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        auto packed = bucket[i];
        auto values = packed.Unpack();

        for (int expr_idx = 0; expr_idx < keys.size(); expr_idx++) {
          proxy::If(program_, !proxy::Equal(values[expr_idx], keys[expr_idx]),
                    [&]() { loop.Continue(i + 1); });
        }

        program_.StoreI8(found_, proxy::Int8(program_, 1).Get());
        // update each aggregator
        for (auto& aggregator : aggregators_) {
          aggregator->Update(values, packed);
        }
        return loop.Continue(size);
      });

  proxy::If(program_, proxy::Int8(program_, program_.LoadI8(found_)) != 1,
            [&]() {
              auto inserted = buffer_->Insert(util::ReferenceVector(keys));

              for (auto& aggregator : aggregators_) {
                aggregator->AddInitialEntry(keys);
              }

              inserted.Pack(util::ReferenceVector(keys));
            });
}

}  // namespace kush::compile