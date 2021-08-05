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
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

std::unique_ptr<ColumnRefExpression> ColRef(const std::unique_ptr<Operator>& op,
                                            std::string_view col,
                                            int child_idx = 0) {
  int idx = op->Schema().GetColumnIndex(col);
  const auto& cols = op->Schema().Columns();
  return std::make_unique<ColumnRefExpression>(cols[idx].Expr().Type(),
                                               child_idx, idx);
}

std::unique_ptr<Expression> ColRefE(const std::unique_ptr<Operator>& op,
                                    std::string_view col, int child_idx = 0) {
  return ColRef(op, col, child_idx);
}

template <typename T>
std::unique_ptr<VirtualColumnRefExpression> VirtColRef(const T& expr, int idx) {
  return std::make_unique<VirtualColumnRefExpression>(expr->Type(), idx);
}

template <typename T>
std::unique_ptr<LiteralExpression> Literal(T t) {
  return std::make_unique<LiteralExpression>(t);
}

std::unique_ptr<BinaryArithmeticExpression> Eq(std::unique_ptr<Expression> e1,
                                               std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::EQ, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Neq(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::NEQ, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Lt(std::unique_ptr<Expression> e1,
                                               std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::LT, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Leq(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::LEQ, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Gt(std::unique_ptr<Expression> e1,
                                               std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::GT, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Geq(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::GEQ, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> EndsWith(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::ENDS_WITH, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> StartsWith(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::STARTS_WITH, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Contains(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::CONTAINS, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> And(
    std::vector<std::unique_ptr<Expression>> expr) {
  std::unique_ptr<BinaryArithmeticExpression> output =
      std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticOperatorType::AND, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<BinaryArithmeticExpression>(
        BinaryArithmeticOperatorType::AND, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<BinaryArithmeticExpression> Or(
    std::vector<std::unique_ptr<Expression>> expr) {
  std::unique_ptr<BinaryArithmeticExpression> output =
      std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticOperatorType::OR, std::move(expr[0]),
          std::move(expr[1]));

  for (int i = 2; i < expr.size(); i++) {
    output = std::make_unique<BinaryArithmeticExpression>(
        BinaryArithmeticOperatorType::OR, std::move(output),
        std::move(expr[i]));
  }

  return output;
}

std::unique_ptr<BinaryArithmeticExpression> Mul(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::MUL, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Div(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::DIV, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Sub(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::SUB, std::move(e1), std::move(e2));
}

std::unique_ptr<BinaryArithmeticExpression> Add(
    std::unique_ptr<Expression> e1, std::unique_ptr<Expression> e2) {
  return std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::ADD, std::move(e1), std::move(e2));
}

std::unique_ptr<AggregateExpression> Sum(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::SUM,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Avg(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::AVG,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Max(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::MAX,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Min(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::MIN,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Count() {
  return std::make_unique<AggregateExpression>(AggregateType::COUNT,
                                               Literal(true));
}

std::unique_ptr<IntToFloatConversionExpression> Float(
    std::unique_ptr<Expression> e) {
  return std::make_unique<IntToFloatConversionExpression>(std::move(e));
}

std::unique_ptr<ExtractExpression> Extract(std::unique_ptr<Expression> e,
                                           ExtractValue v) {
  return std::make_unique<ExtractExpression>(std::move(e), v);
}

std::unique_ptr<CaseExpression> Case(std::unique_ptr<Expression> cond,
                                     std::unique_ptr<Expression> left,
                                     std::unique_ptr<Expression> right) {
  return std::make_unique<CaseExpression>(std::move(cond), std::move(left),
                                          std::move(right));
}