#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "re2/re2.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/extract_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "plan/operator/skinner_join_operator.h"
#include "runtime/date.h"
#include "util/vector_util.h"

namespace kush::util {

std::unique_ptr<kush::plan::ColumnRefExpression> ColRef(
    const std::unique_ptr<kush::plan::Operator>& op, std::string_view col,
    int child_idx = 0) {
  int idx = op->Schema().GetColumnIndex(col);
  const auto& cols = op->Schema().Columns();
  return std::make_unique<kush::plan::ColumnRefExpression>(
      cols[idx].Expr().Type(), cols[idx].Expr().Nullable(), child_idx, idx);
}

std::unique_ptr<kush::plan::Expression> ColRefE(
    const std::unique_ptr<kush::plan::Operator>& op, std::string_view col,
    int child_idx = 0) {
  return ColRef(op, col, child_idx);
}

template <typename T>
std::unique_ptr<kush::plan::VirtualColumnRefExpression> VirtColRef(
    const T& expr, int idx) {
  return std::make_unique<kush::plan::VirtualColumnRefExpression>(
      expr->Type(), expr->Nullable(), idx);
}

std::unique_ptr<kush::plan::VirtualColumnRefExpression> VirtColRef(
    const plan::Expression& expr, int idx) {
  return std::make_unique<kush::plan::VirtualColumnRefExpression>(
      expr.Type(), expr.Nullable(), idx);
}

std::unique_ptr<kush::plan::VirtualColumnRefExpression> VirtColRef(
    const plan::OperatorSchema& schema, std::string_view col) {
  int idx = schema.GetColumnIndex(col);
  const auto& cols = schema.Columns();
  return VirtColRef(cols[idx].Expr(), idx);
}

template <typename T>
std::unique_ptr<kush::plan::Expression> Exp(T t) {
  return t;
}

template <typename T>
std::unique_ptr<kush::plan::LiteralExpression> Literal(T t) {
  return std::make_unique<kush::plan::LiteralExpression>(t);
}

std::unique_ptr<kush::plan::LiteralExpression> Literal(int32_t t,
                                                       std::string_view s) {
  return std::make_unique<kush::plan::LiteralExpression>(t, s);
}

std::unique_ptr<kush::plan::LiteralExpression> Literal(int32_t y, int32_t m,
                                                       int32_t d) {
  return std::make_unique<kush::plan::LiteralExpression>(
      runtime::Date::DateBuilder(y, m, d));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Eq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  if (e1->Type().type_id == catalog::TypeId::ENUM) {
    if (auto l = dynamic_cast<kush::plan::LiteralExpression*>(e2.get())) {
      std::string literal;
      l->Visit(
          nullptr, nullptr, nullptr, nullptr,
          [&](auto x, bool n) { literal = x; }, nullptr, nullptr, nullptr);
      auto l_rewrite = Literal(e1->Type().enum_id, literal);
      return std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticExpressionType::EQ, std::move(e1),
          std::move(l_rewrite));
    }
  }

  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::EQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Neq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  if (e1->Type().type_id == catalog::TypeId::ENUM) {
    if (auto l = dynamic_cast<kush::plan::LiteralExpression*>(e2.get())) {
      std::string literal;
      l->Visit(
          nullptr, nullptr, nullptr, nullptr,
          [&](auto x, bool n) { literal = x; }, nullptr, nullptr, nullptr);
      auto l_rewrite = Literal(e1->Type().enum_id, literal);
      return std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticExpressionType::NEQ, std::move(e1),
          std::move(l_rewrite));
    }
  }

  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::NEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Lt(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::LT, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Leq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::LEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Gt(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::GT, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Geq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::GEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> EndsWith(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::ENDS_WITH, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> StartsWith(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::STARTS_WITH, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Contains(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::CONTAINS, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::RegexpMatchingExpression> Like(
    std::unique_ptr<kush::plan::Expression> e1, std::string_view regexp) {
  return std::make_unique<kush::plan::RegexpMatchingExpression>(
      std::move(e1), std::make_unique<re2::RE2>(regexp));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> And(
    std::vector<std::unique_ptr<kush::plan::Expression>> expr) {
  std::unique_ptr<kush::plan::BinaryArithmeticExpression> output =
      std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticExpressionType::AND, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<kush::plan::BinaryArithmeticExpression>(
        kush::plan::BinaryArithmeticExpressionType::AND, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Or(
    std::vector<std::unique_ptr<kush::plan::Expression>> expr) {
  std::unique_ptr<kush::plan::BinaryArithmeticExpression> output =
      std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticExpressionType::OR, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<kush::plan::BinaryArithmeticExpression>(
        kush::plan::BinaryArithmeticExpressionType::OR, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Mul(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::MUL, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Div(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::DIV, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Sub(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::SUB, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Add(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticExpressionType::ADD, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::AggregateExpression> Sum(
    std::unique_ptr<kush::plan::Expression> expr) {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::SUM, std::move(expr));
}

std::unique_ptr<kush::plan::AggregateExpression> Avg(
    std::unique_ptr<kush::plan::Expression> expr) {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::AVG, std::move(expr));
}

std::unique_ptr<kush::plan::AggregateExpression> Max(
    std::unique_ptr<kush::plan::Expression> expr) {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::MAX, std::move(expr));
}

std::unique_ptr<kush::plan::AggregateExpression> Min(
    std::unique_ptr<kush::plan::Expression> expr) {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::MIN, std::move(expr));
}

std::unique_ptr<kush::plan::AggregateExpression> Count(
    std::unique_ptr<kush::plan::Expression> e) {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::COUNT, std::move(e));
}

std::unique_ptr<kush::plan::AggregateExpression> Count() {
  return std::make_unique<kush::plan::AggregateExpression>(
      kush::plan::AggregateType::COUNT, Literal(true));
}

std::unique_ptr<kush::plan::IntToFloatConversionExpression> Float(
    std::unique_ptr<kush::plan::Expression> e) {
  return std::make_unique<kush::plan::IntToFloatConversionExpression>(
      std::move(e));
}

std::unique_ptr<kush::plan::ExtractExpression> Extract(
    std::unique_ptr<kush::plan::Expression> e, kush::plan::ExtractValue v) {
  return std::make_unique<kush::plan::ExtractExpression>(std::move(e), v);
}

std::unique_ptr<kush::plan::CaseExpression> Case(
    std::unique_ptr<kush::plan::Expression> cond,
    std::unique_ptr<kush::plan::Expression> left,
    std::unique_ptr<kush::plan::Expression> right) {
  return std::make_unique<kush::plan::CaseExpression>(
      std::move(cond), std::move(left), std::move(right));
}

}  // namespace kush::util