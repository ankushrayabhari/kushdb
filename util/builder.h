#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/extract_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "plan/skinner_join_operator.h"
#include "util/vector_util.h"

namespace kush::util {

std::unique_ptr<kush::plan::ColumnRefExpression> ColRef(
    const std::unique_ptr<kush::plan::Operator>& op, std::string_view col,
    int child_idx = 0) {
  int idx = op->Schema().GetColumnIndex(col);
  const auto& cols = op->Schema().Columns();
  return std::make_unique<kush::plan::ColumnRefExpression>(
      cols[idx].Expr().Type(), child_idx, idx);
}

std::unique_ptr<kush::plan::Expression> ColRefE(
    const std::unique_ptr<kush::plan::Operator>& op, std::string_view col,
    int child_idx = 0) {
  return ColRef(op, col, child_idx);
}

template <typename T>
std::unique_ptr<kush::plan::VirtualColumnRefExpression> VirtColRef(
    const T& expr, int idx) {
  return std::make_unique<kush::plan::VirtualColumnRefExpression>(expr->Type(),
                                                                  idx);
}

template <typename T>
std::unique_ptr<kush::plan::LiteralExpression> Literal(T t) {
  return std::make_unique<kush::plan::LiteralExpression>(t);
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Eq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::EQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Neq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::NEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Lt(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::LT, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Leq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::LEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Gt(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::GT, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Geq(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::GEQ, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> EndsWith(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::ENDS_WITH, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> StartsWith(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::STARTS_WITH, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Contains(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::CONTAINS, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Like(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::LIKE, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> And(
    std::vector<std::unique_ptr<kush::plan::Expression>> expr) {
  std::unique_ptr<kush::plan::BinaryArithmeticExpression> output =
      std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticOperatorType::AND, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<kush::plan::BinaryArithmeticExpression>(
        kush::plan::BinaryArithmeticOperatorType::AND, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Or(
    std::vector<std::unique_ptr<kush::plan::Expression>> expr) {
  std::unique_ptr<kush::plan::BinaryArithmeticExpression> output =
      std::make_unique<kush::plan::BinaryArithmeticExpression>(
          kush::plan::BinaryArithmeticOperatorType::OR, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<kush::plan::BinaryArithmeticExpression>(
        kush::plan::BinaryArithmeticOperatorType::OR, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Mul(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::MUL, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Div(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::DIV, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Sub(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::SUB, std::move(e1),
      std::move(e2));
}

std::unique_ptr<kush::plan::BinaryArithmeticExpression> Add(
    std::unique_ptr<kush::plan::Expression> e1,
    std::unique_ptr<kush::plan::Expression> e2) {
  return std::make_unique<kush::plan::BinaryArithmeticExpression>(
      kush::plan::BinaryArithmeticOperatorType::ADD, std::move(e1),
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