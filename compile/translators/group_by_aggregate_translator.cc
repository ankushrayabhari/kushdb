#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
GroupByAggregateTranslator<T>::GroupByAggregateTranslator(
    const plan::GroupByAggregateOperator& group_by_agg,
    proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
    proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs,
    ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      group_by_agg_(group_by_agg),
      program_(program),
      vector_funcs_(vector_funcs),
      hash_funcs_(hash_funcs),
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
  buffer_ = std::make_unique<proxy::HashTable<T>>(program_, vector_funcs_,
                                                  hash_funcs_, packed);

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
void GroupByAggregateTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  // keys = all group by exprs
  std::vector<std::unique_ptr<proxy::Value<T>>> keys;
  for (const auto& group_by : group_by_exprs) {
    keys.push_back(expr_translator_.Compute(group_by.get()));
  }

  auto found = std::make_unique<proxy::Bool<T>>(program_, false);

  // Loop over bucket if exists
  buffer_->Get(util::ReferenceVector(keys), [&](proxy::Struct<T>& packed) {
    auto values = packed.Unpack();

    // TODO: check if the key columns are actually equal

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
          packed.Update(*sum, i);
          break;
        }

        case plan::AggregateType::MIN: {
          auto next = expr_translator_.Compute(agg.Child());
          auto cond = next->EvaluateBinary(
              plan::BinaryArithmeticOperatorType::LT, current_value);

          proxy::If<T>(program_, static_cast<proxy::Bool<T>&>(*cond),
                       [&]() { packed.Update(*next, i); });
          break;
        }

        case plan::AggregateType::MAX: {
          auto next = expr_translator_.Compute(agg.Child());
          auto cond = next->EvaluateBinary(
              plan::BinaryArithmeticOperatorType::GT, current_value);

          proxy::If<T>(program_, static_cast<proxy::Bool<T>&>(*cond),
                       [&]() { packed.Update(*next, i); });
          break;
        }

        case plan::AggregateType::AVG: {
          auto next = expr_translator_.Compute(agg.Child());
          auto sub = next->EvaluateBinary(
              plan::BinaryArithmeticOperatorType::SUB, current_value);
          auto to_add = sub->EvaluateBinary(
              plan::BinaryArithmeticOperatorType::DIV, record_count_field);
          auto sum = current_value.EvaluateBinary(
              plan::BinaryArithmeticOperatorType::ADD, *to_add);
          packed.Update(*sum, i);
          break;
        }

        case plan::AggregateType::COUNT: {
          auto one = proxy::Int64<T>(program_, 1);
          auto sum = current_value.EvaluateBinary(
              plan::BinaryArithmeticOperatorType::ADD, one);
          packed.Update(*sum, i);
          break;
        }
      }
    }

    // increment record counter field
    auto one = proxy::Int64<T>(program_, 1);
    auto sum = record_count_field.EvaluateBinary(
        plan::BinaryArithmeticOperatorType::ADD, one);
    packed.Update(*sum, 0);
  });

  proxy::If<T>(program_, !(*found), [&]() {
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
        case plan::AggregateType::SUM:
        case plan::AggregateType::AVG:
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