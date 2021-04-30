#include "compile/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/function.h"
#include "compile/proxy/int.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/order_by_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
OrderByTranslator<T>::OrderByTranslator(
    const plan::OrderByOperator& order_by, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(order_by, std::move(children)),
      order_by_(order_by),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void OrderByTranslator<T>::Produce() {
  // include every child column inside the struct
  proxy::StructBuilder<T> packed(program_);
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type());
  }
  packed.Build();

  // init vector
  buffer_ = std::make_unique<proxy::Vector<T>>(program_, packed);

  // populate vector
  this->Child().Produce();

  // sort
  proxy::ComparisonFunction<T> comp_fn(
      program_, packed,
      [&](proxy::Struct<T>& s1, proxy::Struct<T>& s2,
          std::function<void(proxy::Bool<T>)> Return) {
        auto s1_fields = s1.Unpack();
        auto s2_fields = s2.Unpack();

        const auto sort_keys = order_by_.KeyExprs();
        const auto& ascending = order_by_.Ascending();
        for (int i = 0; i < sort_keys.size(); i++) {
          int field_idx = sort_keys[i].get().GetColumnIdx();

          auto& s1_field = *s1_fields[field_idx];
          auto& s2_field = *s2_fields[field_idx];
          auto asc = ascending[i];

          if (asc) {
            auto v1 = s1_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s2_field);
            proxy::If(program_, static_cast<proxy::Bool<T>&>(*v1),
                      [&]() { Return(proxy::Bool<T>(program_, true)); });

            auto v2 = s2_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s1_field);
            proxy::If(program_, static_cast<proxy::Bool<T>&>(*v2),
                      [&]() { Return(proxy::Bool<T>(program_, false)); });
          } else {
            auto v1 = s1_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s2_field);
            proxy::If(program_, static_cast<proxy::Bool<T>&>(*v1),
                      [&]() { Return(proxy::Bool<T>(program_, false)); });

            auto v2 = s2_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s1_field);
            proxy::If(program_, static_cast<proxy::Bool<T>&>(*v2),
                      [&]() { Return(proxy::Bool<T>(program_, true)); });
          }
        }

        Return(proxy::Bool<T>(program_, false));
      });

  buffer_->Sort(comp_fn.Get());

  proxy::Loop<T>(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32<T>(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32<T>>(0);
        return i < buffer_->Size();
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32<T>>(0);

        this->Child().SchemaValues().SetValues((*buffer_)[i].Unpack());

        this->values_.ResetValues();
        for (const auto& column : order_by_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        std::unique_ptr<proxy::Value<T>> next_i = (i + 1).ToPointer();
        return util::MakeVector(std::move(next_i));
      });

  buffer_->Reset();
}

template <typename T>
void OrderByTranslator<T>::Consume(OperatorTranslator<T>& src) {
  buffer_->PushBack().Pack(this->Child().SchemaValues().Values());
}

INSTANTIATE_ON_IR(OrderByTranslator);

}  // namespace kush::compile