#include "compile/translators/hash_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/if.h"
#include "compile/proxy/value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
HashJoinTranslator<T>::HashJoinTranslator(
    const plan::HashJoinOperator& hash_join, ProgramBuilder<T>& program,
    proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
    proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      hash_join_(hash_join),
      program_(program),
      vector_funcs_(vector_funcs),
      hash_funcs_(hash_funcs),
      expr_translator_(program_, *this) {}

template <typename T>
void HashJoinTranslator<T>::Produce() {
  // Struct for all columns in the left tuple
  proxy::StructBuilder<T> packed(program_);
  const auto& child_schema = hash_join_.LeftChild().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type());
  }
  packed.Build();

  buffer_ = std::make_unique<proxy::HashTable<T>>(program_, vector_funcs_,
                                                  hash_funcs_, packed);

  this->LeftChild().Produce();
  this->RightChild().Produce();

  buffer_.reset();
}

template <typename T>
void HashJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto& left_translator = this->LeftChild();
  const auto left_keys = hash_join_.LeftColumns();
  const auto right_keys = hash_join_.RightColumns();

  // Build side
  if (&src == &left_translator) {
    std::vector<std::unique_ptr<proxy::Value<T>>> key_columns;
    for (const auto& left_key : left_keys) {
      key_columns.push_back(expr_translator_.Compute(left_key.get()));
    }

    auto entry = buffer_->Insert(util::ReferenceVector(key_columns));
    entry.Pack(left_translator.SchemaValues().Values());
    return;
  }

  // Probe Side
  std::vector<std::unique_ptr<proxy::Value<T>>> key_columns;
  for (const auto& right_key : right_keys) {
    key_columns.push_back(expr_translator_.Compute(right_key.get()));
  }
  buffer_->Get(
      util::ReferenceVector(key_columns), [&](proxy::Struct<T>& left_tuple) {
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

        auto cond = expr_translator_.Compute(*conj);
        proxy::If(program_, dynamic_cast<proxy::Bool<T>&>(*cond), [&]() {
          for (const auto& column : hash_join_.Schema().Columns()) {
            this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
          }

          if (auto parent = this->Parent()) {
            parent->get().Consume(*this);
          }
        });
      });
}

INSTANTIATE_ON_IR(HashJoinTranslator);

}  // namespace kush::compile