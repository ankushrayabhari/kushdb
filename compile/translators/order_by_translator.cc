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
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/order_by_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

OrderByTranslator::OrderByTranslator(
    const plan::OrderByOperator& order_by, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder, execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(order_by, std::move(children)),
      order_by_(order_by),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program, *this) {}

void OrderByTranslator::Produce(proxy::Pipeline& output) {
  // buffer the input
  proxy::StructBuilder packed(program_);
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type(), col.Expr().Nullable());
  }
  packed.Build();
  buffer_ = std::make_unique<proxy::Vector>(program_, state_, packed);

  // populate buffer
  proxy::Pipeline input(program_, pipeline_builder_);
  input.Init([&]() { buffer_->Init(); });
  input.Reset([&]() { buffer_->Reset(); });
  input.Size([&]() { return buffer_->Size(); });
  this->Child().Produce(input);
  input.Build();

  // sort the buffer
  proxy::ComparisonFunction comp_fn(
      program_, packed,
      [&](proxy::Struct& s1, proxy::Struct& s2,
          std::function<void(proxy::Bool)> Return) {
        const auto sort_keys = order_by_.KeyExprs();
        const auto& ascending = order_by_.Ascending();
        for (int i = 0; i < sort_keys.size(); i++) {
          int field_idx = sort_keys[i].get().GetColumnIdx();

          auto s1_field = s1.Get(field_idx);
          auto s2_field = s2.Get(field_idx);
          auto asc = ascending[i];

          proxy::If(program_, LessThan(s1_field, s2_field),
                    [&]() { Return(proxy::Bool(program_, asc)); });

          proxy::If(program_, LessThan(s2_field, s1_field),
                    [&]() { Return(proxy::Bool(program_, !asc)); });
        }

        Return(proxy::Bool(program_, false));
      });
  proxy::Pipeline sort(program_, pipeline_builder_);
  sort.Body([&]() { buffer_->Sort(comp_fn.Get()); });
  sort.Build();
  sort.Get().AddPredecessor(input.Get());

  output.Get().AddPredecessor(sort.Get());
  output.Body(input, [&](proxy::Int32 start, proxy::Int32 end) {
    proxy::Loop(
        program_, [&](auto& loop) { loop.AddLoopVariable(start); },
        [&](auto& loop) {
          auto i = loop.template GetLoopVariable<proxy::Int32>(0);
          return i <= end;
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
  });
}

void OrderByTranslator::Consume(OperatorTranslator& src) {
  buffer_->PushBack().Pack(this->Child().SchemaValues().Values());
}

}  // namespace kush::compile