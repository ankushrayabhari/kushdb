#include "compile/translators/hash_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/hash_join_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    const plan::HashJoinOperator& hash_join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder, execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(hash_join, std::move(children)),
      hash_join_(hash_join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(program_, *this) {}

void HashJoinTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto& pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
  // Struct for all columns in the left tuple
  proxy::StructBuilder packed(program_);
  const auto& child_schema = hash_join_.LeftChild().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type(), col.Expr().Nullable());
  }
  packed.Build();

  buffer_ = std::make_unique<proxy::HashTable>(program_, state_, packed);
  all_not_null_ptr_ = program_.Global(program_.I8Type(), program_.ConstI8(0));

  this->LeftChild().Produce();

  program_.Return();
  auto child_pipeline = pipeline_builder_.FinishPipeline();
  pipeline_builder_.GetCurrentPipeline().AddPredecessor(
      std::move(child_pipeline));

  // Loop over elements of HT and output row
  program_.SetCurrentBlock(output_pipeline_func);
  this->RightChild().Produce();
  buffer_->Reset();
}

void CheckEquality(khir::ProgramBuilder& program,
                   ExpressionTranslator& expr_translator,
                   const std::vector<std::reference_wrapper<
                       const kush::plan::ColumnRefExpression>>& left_keys,
                   const std::vector<std::reference_wrapper<
                       const kush::plan::ColumnRefExpression>>& right_keys,
                   std::function<void()> true_case, int i = 0) {
  if (i == left_keys.size()) {
    // all of them panned out so do the true case
    true_case();
    return;
  }

  auto lhs = expr_translator.Compute(left_keys[i]);
  auto rhs = expr_translator.Compute(right_keys[i]);

  proxy::If(program, proxy::Equal(lhs, rhs), [&]() {
    CheckEquality(program, expr_translator, left_keys, right_keys, true_case,
                  i + 1);
  });
}

void HashJoinTranslator::Consume(OperatorTranslator& src) {
  auto& left_translator = this->LeftChild();
  const auto left_keys = hash_join_.LeftColumns();
  const auto right_keys = hash_join_.RightColumns();

  // Build side
  if (&src == &left_translator) {
    program_.StoreI8(all_not_null_ptr_, proxy::Int8(program_, 1).Get());
    std::vector<proxy::SQLValue> key_columns;
    for (const auto& left_key : left_keys) {
      auto val = expr_translator_.Compute(left_key.get());
      key_columns.push_back(val);
      proxy::If(program_, val.IsNull(), [&]() {
        program_.StoreI8(all_not_null_ptr_, proxy::Int8(program_, 0).Get());
      });
    }

    proxy::If(program_,
              proxy::Int8(program_, program_.LoadI8(all_not_null_ptr_)) != 0,
              [&]() {
                auto entry = buffer_->Insert(key_columns);
                entry.Pack(left_translator.SchemaValues().Values());
              });
    return;
  }

  // Probe Side
  std::vector<proxy::SQLValue> key_columns;
  for (const auto& right_key : right_keys) {
    key_columns.push_back(expr_translator_.Compute(right_key.get()));
  }

  auto bucket = buffer_->Get(key_columns);

  proxy::Loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < bucket.Size();
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        auto left_tuple = bucket[i];
        left_translator.SchemaValues().SetValues(left_tuple.Unpack());

        CheckEquality(program_, expr_translator_, left_keys, right_keys, [&]() {
          this->values_.ResetValues();
          for (const auto& column : hash_join_.Schema().Columns()) {
            this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
          }

          if (auto parent = this->Parent()) {
            parent->get().Consume(*this);
          }
        });

        return loop.Continue(i + 1);
      });
}

}  // namespace kush::compile