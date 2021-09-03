#include "compile/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/function.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/order_by_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

OrderByTranslator::OrderByTranslator(
    const plan::OrderByOperator& order_by, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(order_by, std::move(children)),
      order_by_(order_by),
      program_(program),
      expr_translator_(program, *this) {}

void OrderByTranslator::Produce() {
  // include every child column inside the struct
  proxy::StructBuilder packed(program_);
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    packed.Add(col.Expr().Type());
  }
  packed.Build();

  // init vector
  buffer_ = std::make_unique<proxy::Vector>(program_, packed, false);

  // populate vector
  this->Child().Produce();

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

          auto& s1_field = *s1_fields[field_idx];
          auto& s2_field = *s2_fields[field_idx];
          auto asc = ascending[i];

          if (asc) {
            auto v1 = s1_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s2_field);
            proxy::Ternary(
                program_, static_cast<proxy::Bool&>(*v1),
                [&]() -> std::vector<khir::Value> {
                  Return(proxy::Bool(program_, true));
                  return {};
                },
                [&]() -> std::vector<khir::Value> { return {}; });

            auto v2 = s2_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s1_field);
            proxy::Ternary(
                program_, static_cast<proxy::Bool&>(*v2),
                [&]() -> std::vector<khir::Value> {
                  Return(proxy::Bool(program_, false));
                  return {};
                },
                [&]() -> std::vector<khir::Value> { return {}; });
          } else {
            auto v1 = s1_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s2_field);
            proxy::Ternary(
                program_, static_cast<proxy::Bool&>(*v1),
                [&]() -> std::vector<khir::Value> {
                  Return(proxy::Bool(program_, false));
                  return {};
                },
                [&]() -> std::vector<khir::Value> { return {}; });

            auto v2 = s2_field.EvaluateBinary(
                plan::BinaryArithmeticOperatorType::LT, s1_field);
            proxy::Ternary(
                program_, static_cast<proxy::Bool&>(*v2),
                [&]() -> std::vector<khir::Value> {
                  Return(proxy::Bool(program_, true));
                  return {};
                },
                [&]() -> std::vector<khir::Value> { return {}; });
          }
        }

        Return(proxy::Bool(program_, false));
      });

  buffer_->Sort(comp_fn.Get());

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