#include "compile/translators/hash_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/value/value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/hash_join_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    const plan::HashJoinOperator& hash_join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(hash_join, std::move(children)),
      hash_join_(hash_join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program_, *this) {}

void HashJoinTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto& pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
  // Struct for all columns in the left tuple
  proxy::StructBuilder packed(program_);
  const auto& child_schema = hash_join_.LeftChild().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type());
  }
  packed.Build();

  buffer_ = std::make_unique<proxy::HashTable>(program_, packed);

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

void HashJoinTranslator::Consume(OperatorTranslator& src) {
  auto& left_translator = this->LeftChild();
  const auto left_keys = hash_join_.LeftColumns();
  const auto right_keys = hash_join_.RightColumns();

  // Build side
  if (&src == &left_translator) {
    std::vector<std::unique_ptr<proxy::IRValue>> key_columns;
    for (const auto& left_key : left_keys) {
      key_columns.push_back(expr_translator_.Compute(left_key.get()));
    }

    auto entry = buffer_->Insert(util::ReferenceVector(key_columns));
    entry.Pack(left_translator.SchemaValues().Values());
    return;
  }

  // Probe Side
  std::vector<std::unique_ptr<proxy::IRValue>> key_columns;
  for (const auto& right_key : right_keys) {
    key_columns.push_back(expr_translator_.Compute(right_key.get()));
  }

  auto bucket = buffer_->Get(util::ReferenceVector(key_columns));

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

        // construct boolean expression that compares each left column to right
        // column
        std::unique_ptr<plan::Expression> conj;
        for (int i = 0; i < left_keys.size(); i++) {
          const auto& left_key = left_keys[i].get();
          const auto& right_key = right_keys[i].get();

          auto eq = std::make_unique<plan::BinaryArithmeticExpression>(
              plan::BinaryArithmeticOperatorType::EQ,
              std::make_unique<plan::ColumnRefExpression>(
                  left_key.Type(), left_key.GetChildIdx(),
                  left_key.GetColumnIdx()),
              std::make_unique<plan::ColumnRefExpression>(
                  right_key.Type(), right_key.GetChildIdx(),
                  right_key.GetColumnIdx()));

          if (conj == nullptr) {
            conj = std::move(eq);
          } else {
            conj = std::make_unique<plan::BinaryArithmeticExpression>(
                plan::BinaryArithmeticOperatorType::AND, std::move(conj),
                std::move(eq));
          }
        }

        auto cond = expr_translator_.template ComputeAs<proxy::Bool>(*conj);
        proxy::If(program_, cond, [&]() {
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