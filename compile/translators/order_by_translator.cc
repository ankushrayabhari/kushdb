#include "compile/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/function.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/order_by_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

OrderByTranslator::OrderByTranslator(
    const plan::OrderByOperator& order_by, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(order_by, std::move(children)),
      order_by_(order_by),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program, *this) {}

void OrderByTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto& child_pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, child_pipeline.Name());
  // include every child column inside the struct
  proxy::StructBuilder packed(program_);
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type(), col.Expr().Nullable());
  }
  packed.Build();

  // init vector
  buffer_ = std::make_unique<proxy::Vector>(program_, packed);

  // populate vector
  this->Child().Produce();

  program_.Return();
  auto child_pipeline_finished = pipeline_builder_.FinishPipeline();

  auto& sort_pipeline = pipeline_builder_.CreatePipeline();
  sort_pipeline.AddPredecessor(std::move(child_pipeline_finished));
  program_.CreatePublicFunction(program_.VoidType(), {}, sort_pipeline.Name());
  // sort
  proxy::ComparisonFunction comp_fn(
      program_, packed,
      [&](proxy::Struct& s1, proxy::Struct& s2,
          std::function<void(proxy::Bool)> Return) {
        auto s1_fields = s1.Unpack();
        auto s2_fields = s2.Unpack();

        const auto sort_keys = order_by_.KeyExprs();
        const auto& ascending = order_by_.Ascending();
        for (int i = 0; i < sort_keys.size(); i++) {
          int field_idx = sort_keys[i].get().GetColumnIdx();

          auto& s1_field = s1_fields[field_idx];
          auto& s2_field = s2_fields[field_idx];
          auto asc = ascending[i];

          proxy::If(program_, LessThan(s1_field, s2_field),
                    [&]() { Return(proxy::Bool(program_, asc)); });

          proxy::If(program_, LessThan(s2_field, s1_field),
                    [&]() { Return(proxy::Bool(program_, !asc)); });
        }

        Return(proxy::Bool(program_, false));
      });

  buffer_->Sort(comp_fn.Get());
  program_.Return();

  auto sort_pipeline_finished = pipeline_builder_.FinishPipeline();
  pipeline_builder_.GetCurrentPipeline().AddPredecessor(
      std::move(sort_pipeline_finished));
  program_.SetCurrentBlock(output_pipeline_func);
  // Loop over elements of HT and output row
  proxy::Loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < buffer_->Size();
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        this->Child().SchemaValues().SetValues((*buffer_)[i].Unpack());

        this->values_.ResetValues();
        for (const auto& column : order_by_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  buffer_->Reset();
}

void OrderByTranslator::Consume(OperatorTranslator& src) {
  buffer_->PushBack().Pack(this->Child().SchemaValues().Values());
}

}  // namespace kush::compile