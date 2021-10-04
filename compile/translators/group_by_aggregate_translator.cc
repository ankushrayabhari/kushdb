#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/value.h"
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
  {
    // Include a field for counting # of records per group
    packed.Add(catalog::SqlType::BIGINT);

    // Include every group by column inside the struct
    for (const auto& group_by : group_by_exprs) {
      packed.Add(group_by.get().Type());
    }

    // Include every aggregate inside the struct
    for (const auto& agg : agg_exprs) {
      packed.Add(agg.get().Type());
    }
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

    // everything except the group row counter is a virtual column
    for (int i = 1; i < values.size(); i++) {
      this->virtual_values_.AddVariable(std::move(values[i]));
    }

    // generate output variables
    this->values_.ResetValues();
    for (const auto& column : group_by_agg_.Schema().Columns()) {
      this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  });
  buffer_->Reset();
}

void CheckEquality(
    int i, khir::ProgramBuilder& program, ExpressionTranslator& expr_translator,
    std::vector<std::reference_wrapper<proxy::Value>>& values,
    std::vector<std::reference_wrapper<const kush::plan::Expression>>&
        group_by_exprs,
    std::function<void()> true_case) {
  if (i == group_by_exprs.size()) {
    // all of them panned out so do the true case
    true_case();
    return;
  }

  auto& lhs = values[i + 1].get();
  auto rhs = expr_translator.Compute(group_by_exprs[i]);
  auto cond = lhs.EvaluateBinary(plan::BinaryArithmeticOperatorType::EQ, *rhs);

  proxy::If(program, static_cast<proxy::Bool&>(*cond), [&]() {
    CheckEquality(i + 1, program, expr_translator, values, group_by_exprs,
                  true_case);
  });
}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // keys = all group by exprs
  std::vector<std::unique_ptr<proxy::Value>> keys;
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

        auto values_ref = util::ReferenceVector(values);
        CheckEquality(
            0, program_, expr_translator_, values_ref, group_by_exprs, [&]() {
              program_.StoreI8(found_, proxy::Int8(program_, 1).Get());

              auto& record_count_field = *values[0];

              // Update aggregations.
              for (int i = 1 + group_by_exprs.size(); i < values.size(); i++) {
                auto& agg = agg_exprs[i - (1 + group_by_exprs.size())].get();
                auto& current_value = *values[i];

                switch (agg.AggType()) {
                  case plan::AggregateType::SUM: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto sum = current_value.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::ADD, *next);
                    packed.Update(i, *sum);
                    break;
                  }

                  case plan::AggregateType::MIN: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto cond = next->EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::LT, current_value);

                    proxy::If(program_, static_cast<proxy::Bool&>(*cond),
                              [&]() { packed.Update(i, *next); });
                    break;
                  }

                  case plan::AggregateType::MAX: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto cond = next->EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::GT, current_value);

                    proxy::If(program_, static_cast<proxy::Bool&>(*cond),
                              [&]() { packed.Update(i, *next); });
                    break;
                  }

                  case plan::AggregateType::AVG: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto as_float = this->ToFloat(*next);

                    auto sub = as_float.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::SUB, current_value);

                    auto record_count_as_float =
                        this->ToFloat(record_count_field);
                    auto to_add = sub->EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::DIV,
                        record_count_as_float);
                    auto sum = current_value.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::ADD, *to_add);
                    packed.Update(i, *sum);
                    break;
                  }

                  case plan::AggregateType::COUNT: {
                    auto one = proxy::Int64(program_, 1);
                    auto sum = current_value.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::ADD, one);
                    packed.Update(i, *sum);
                    break;
                  }
                }
              }

              // increment record counter field
              auto one = proxy::Int64(program_, 1);
              auto sum = record_count_field.EvaluateBinary(
                  plan::BinaryArithmeticOperatorType::ADD, one);
              packed.Update(0, *sum);
            });

        // If we didn't find, move to next element
        // Else, break out of loop
        std::unique_ptr<proxy::Int32> next_index;
        proxy::Int32 next = proxy::Ternary(
            program_, proxy::Int8(program_, program_.LoadI8(found_)) != 1,
            [&]() { return i + 1; }, [&]() { return size; });
        return loop.Continue(next);
      });

  proxy::If(
      program_, proxy::Int8(program_, program_.LoadI8(found_)) != 1, [&]() {
        auto inserted = buffer_->Insert(util::ReferenceVector(keys));

        std::vector<std::unique_ptr<proxy::Value>> values;

        // Record Counter
        values.push_back(std::make_unique<proxy::Int64>(program_, 2));

        // Group By Values
        for (auto& group_by : group_by_exprs) {
          values.push_back(expr_translator_.Compute(group_by.get()));
        }

        // Aggregates
        for (auto& agg : agg_exprs) {
          switch (agg.get().AggType()) {
            case plan::AggregateType::AVG: {
              auto v = expr_translator_.Compute(agg.get().Child());
              values.push_back(this->ToFloat(*v).ToPointer());
              break;
            }

            case plan::AggregateType::SUM:
            case plan::AggregateType::MAX:
            case plan::AggregateType::MIN:
              values.push_back(expr_translator_.Compute(agg.get().Child()));
              break;
            case plan::AggregateType::COUNT:
              values.push_back(std::make_unique<proxy::Int64>(program_, 1));
              break;
          }
        }

        inserted.Pack(util::ReferenceVector(values));
      });
}

proxy::Float64 GroupByAggregateTranslator::ToFloat(proxy::Value& v) {
  if (auto x = dynamic_cast<proxy::Float64*>(&v)) {
    return proxy::Float64(*x);
  }

  if (auto x = dynamic_cast<proxy::Int8*>(&v)) {
    return proxy::Float64(program_, *x);
  }

  if (auto x = dynamic_cast<proxy::Int16*>(&v)) {
    return proxy::Float64(program_, *x);
  }

  if (auto x = dynamic_cast<proxy::Int32*>(&v)) {
    return proxy::Float64(program_, *x);
  }

  if (auto x = dynamic_cast<proxy::Int64*>(&v)) {
    return proxy::Float64(program_, *x);
  }

  throw std::runtime_error("Can't convert non-integral type to float.");
}

}  // namespace kush::compile