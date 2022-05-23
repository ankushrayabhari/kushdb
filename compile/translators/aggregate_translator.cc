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
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

AggregateTranslator::AggregateTranslator(
    const plan::AggregateOperator& agg, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder, execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(agg, std::move(children)),
      agg_(agg),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program, *this) {}

void AggregateTranslator::Produce(proxy::Pipeline& output) {
  agg_struct_ = std::make_unique<proxy::StructBuilder>(program_);
  for (const auto& agg : agg_.AggExprs()) {
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
      program_.PointerCast(
          program_.ConstPtr(state_.Allocate(program_.GetSize(type))),
          program_.PointerType(type)));
  empty_value_ = program_.PointerCast(
      program_.ConstPtr(state_.Allocate(program_.GetSize(program_.I64Type()))),
      program_.PointerType(program_.I64Type()));

  // Fill aggregators
  proxy::Pipeline input(program_, pipeline_builder_);
  input.Init([&]() { program_.StoreI64(empty_value_, program_.ConstI64(0)); });
  this->Child().Produce(input);
  input.Build();

  output.Get().AddPredecessor(input.Get());
  output.Body([&]() {
    proxy::Int64 empty(program_, program_.LoadI64(empty_value_));
    proxy::If(
        program_, empty == 0,
        [&]() {
          this->values_.PopulateWithNull(program_, agg_.Schema());
          if (auto parent = this->Parent()) {
            parent->get().Consume(*this);
          }
        },
        [&]() {
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