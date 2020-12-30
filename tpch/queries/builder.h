#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/string_comparison_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile::cpp;
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

std::unique_ptr<ArithmeticExpression> Mul(std::unique_ptr<Expression> e1,
                                          std::unique_ptr<Expression> e2) {
  return std::make_unique<ArithmeticExpression>(ArithmeticOperatorType::MUL,
                                                std::move(e1), std::move(e2));
}

std::unique_ptr<ArithmeticExpression> Sub(std::unique_ptr<Expression> e1,
                                          std::unique_ptr<Expression> e2) {
  return std::make_unique<ArithmeticExpression>(ArithmeticOperatorType::SUB,
                                                std::move(e1), std::move(e2));
}

std::unique_ptr<ArithmeticExpression> Add(std::unique_ptr<Expression> e1,
                                          std::unique_ptr<Expression> e2) {
  return std::make_unique<ArithmeticExpression>(ArithmeticOperatorType::ADD,
                                                std::move(e1), std::move(e2));
}

std::unique_ptr<AggregateExpression> Sum(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::SUM,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Avg(std::unique_ptr<Expression> expr) {
  return std::make_unique<AggregateExpression>(AggregateType::AVG,
                                               std::move(expr));
}

std::unique_ptr<AggregateExpression> Count() {
  return std::make_unique<AggregateExpression>(AggregateType::AVG,
                                               Literal(true));
}