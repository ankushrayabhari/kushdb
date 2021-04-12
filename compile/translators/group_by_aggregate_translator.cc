#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/float.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/struct.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
GroupByAggregateTranslator<T>::GroupByAggregateTranslator(
    const plan::GroupByAggregateOperator& group_by_agg,
    ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(group_by_agg, std::move(children)),
      group_by_agg_(group_by_agg),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void GroupByAggregateTranslator<T>::Produce() {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // Struct for hash table
  proxy::StructBuilder<T> packed(program_);
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
  buffer_ = std::make_unique<proxy::HashTable<T>>(program_, packed);

  // Declare the found variable
  found_ = std::make_unique<proxy::Ptr<T, proxy::Bool<T>>>(
      program_, program_.Alloca(program_.I1Type()));

  // Populate hash table
  this->Child().Produce();

  // Loop over elements of HT and output row
  buffer_->ForEach([&](proxy::Struct<T>& packed) {
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

  buffer_.reset();
}

template <typename T>
void CheckEquality(
    int i, ProgramBuilder<T>& program, ExpressionTranslator<T>& expr_translator,
    std::vector<std::reference_wrapper<proxy::Value<T>>>& values,
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

  proxy::If<T>(program, static_cast<proxy::Bool<T>&>(*cond), [&]() {
    CheckEquality(i + 1, program, expr_translator, values, group_by_exprs,
                  true_case);
  });
}

template <typename T>
void GroupByAggregateTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // keys = all group by exprs
  std::vector<std::unique_ptr<proxy::Value<T>>> keys;
  for (const auto& group_by : group_by_exprs) {
    keys.push_back(expr_translator_.Compute(group_by.get()));
  }

  found_->Store(proxy::Bool<T>(program_, false));

  // Loop over bucket if exists
  auto bucket = buffer_->Get(util::ReferenceVector(keys));
  auto size = bucket.Size();
  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::UInt32<T>(program_, 0); },
      [&](proxy::UInt32<T>& i) { return i < size; },
      [&](proxy::UInt32<T>& i) {
        auto packed = bucket[i];
        auto values = packed.Unpack();

        auto values_ref = util::ReferenceVector(values);
        CheckEquality<T>(
            0, program_, expr_translator_, values_ref, group_by_exprs, [&]() {
              found_->Store(proxy::Bool<T>(program_, true));

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

                    proxy::If<T>(program_, static_cast<proxy::Bool<T>&>(*cond),
                                 [&]() { packed.Update(i, *next); });
                    break;
                  }

                  case plan::AggregateType::MAX: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto cond = next->EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::GT, current_value);

                    proxy::If<T>(program_, static_cast<proxy::Bool<T>&>(*cond),
                                 [&]() { packed.Update(i, *next); });
                    break;
                  }

                  case plan::AggregateType::AVG: {
                    auto next = expr_translator_.Compute(agg.Child());
                    auto as_float = proxy::Float64<T>(
                        program_, program_.CastSignedIntToF64(next->Get()));

                    auto sub = as_float.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::SUB, current_value);

                    auto record_count_as_float = proxy::Float64<T>(
                        program_,
                        program_.CastSignedIntToF64(record_count_field.Get()));
                    auto to_add = sub->EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::DIV,
                        record_count_as_float);
                    auto sum = current_value.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::ADD, *to_add);
                    packed.Update(i, *sum);
                    break;
                  }

                  case plan::AggregateType::COUNT: {
                    auto one = proxy::Int64<T>(program_, 1);
                    auto sum = current_value.EvaluateBinary(
                        plan::BinaryArithmeticOperatorType::ADD, one);
                    packed.Update(i, *sum);
                    break;
                  }
                }
              }

              // increment record counter field
              auto one = proxy::Int64<T>(program_, 1);
              auto sum = record_count_field.EvaluateBinary(
                  plan::BinaryArithmeticOperatorType::ADD, one);
              packed.Update(0, *sum);
            });

        // If we didn't find, move to next element
        // Else, break out of loop
        std::unique_ptr<proxy::UInt32<T>> next_index;
        proxy::If<T> check(program_, !found_->Load(), [&]() {
          next_index = std::make_unique<proxy::UInt32<T>>(
              i + proxy::UInt32<T>(program_, 1));
        });
        return proxy::UInt32<T>(program_, check.Phi(size, *next_index));
      });

  proxy::If<T>(program_, !found_->Load(), [&]() {
    auto inserted = buffer_->Insert(util::ReferenceVector(keys));

    std::vector<std::unique_ptr<proxy::Value<T>>> values;

    // Record Counter
    values.push_back(std::make_unique<proxy::Int64<T>>(program_, 2));

    // Group By Values
    for (auto& group_by : group_by_exprs) {
      values.push_back(expr_translator_.Compute(group_by.get()));
    }

    // Aggregates
    for (auto& agg : agg_exprs) {
      switch (agg.get().AggType()) {
        case plan::AggregateType::AVG: {
          auto v = expr_translator_.Compute(agg.get().Child());
          values.push_back(std::make_unique<proxy::Float64<T>>(
              program_, program_.CastSignedIntToF64(v->Get())));
          break;
        }

        case plan::AggregateType::SUM:
        case plan::AggregateType::MAX:
        case plan::AggregateType::MIN:
          values.push_back(expr_translator_.Compute(agg.get().Child()));
          break;
        case plan::AggregateType::COUNT:
          values.push_back(std::make_unique<proxy::Int64<T>>(program_, 1));
          break;
      }
    }

    inserted.Pack(util::ReferenceVector(values));
  });
}

INSTANTIATE_ON_IR(GroupByAggregateTranslator);

}  // namespace kush::compile