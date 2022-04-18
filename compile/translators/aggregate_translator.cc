#include "compile/translators/aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

AggregateTranslator::AggregateTranslator(
    const plan::AggregateOperator& agg, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(agg, std::move(children)),
      agg_(agg),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program, *this) {}

void AggregateTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto& pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
  auto agg_exprs = agg_.AggExprs();

  agg_struct_ = std::make_unique<proxy::StructBuilder>(program_);

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

    aggregators_.back()->AddFields(*agg_struct_);
  }
  agg_struct_->Build();
  auto type = agg_struct_->Type();

  value_ = std::make_unique<proxy::Struct>(
      program_, *agg_struct_,
      program_.Global(
          false, true, type,
          program_.ConstantStruct(type, agg_struct_->DefaultValues())));
  empty_value_ =
      program_.Global(false, true, program_.I64Type(), program_.ConstI64(0));

  // Fill aggregators
  this->Child().Produce();

  // Finish pipeline
  program_.Return();
  auto child_pipeline = pipeline_builder_.FinishPipeline();
  pipeline_builder_.GetCurrentPipeline().AddPredecessor(
      std::move(child_pipeline));
  program_.SetCurrentBlock(output_pipeline_func);

  // Loop over elements of HT and output row
  proxy::Int64 empty(program_, program_.LoadI64(empty_value_));
  proxy::If(program_, empty != 0, [&]() {
    this->virtual_values_.ResetValues();
    for (const auto& agg : aggregators_) {
      this->virtual_values_.AddVariable(agg->Get(*value_));
    }

    // generate output variables
    this->values_.ResetValues();
    for (const auto& column : agg_.Schema().Columns()) {
      auto val = expr_translator_.Compute(column.Expr());
      this->values_.AddVariable(val);
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  });
}

void AggregateTranslator::Consume(OperatorTranslator& src) {
  proxy::Int64 empty(program_, program_.LoadI64(empty_value_));
  proxy::If(
      program_, empty == 0,
      [&]() {
        for (const auto& agg : aggregators_) {
          agg->Initialize(*value_);
        }

        program_.StoreI64(empty_value_, program_.ConstI64(1));
      },
      [&]() {
        for (const auto& agg : aggregators_) {
          agg->Update(*value_);
        }
      });
}

}  // namespace kush::compile