#pragma once

#include <vector>

#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/extract_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile {

class PredicateColumnCollector : public plan::ImmutableExpressionVisitor {
 public:
  PredicateColumnCollector() = default;
  virtual ~PredicateColumnCollector() = default;

  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
  PredicateColumns();

  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::UnaryArithmeticExpression& arith) override;
  void Visit(const plan::BinaryArithmeticExpression& arith) override;
  void Visit(const plan::CaseExpression& case_expr) override;
  void Visit(const plan::IntToFloatConversionExpression& conv) override;
  void Visit(const plan::ExtractExpression& conv) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::RegexpMatchingExpression& match) override;
  void Visit(const plan::EnumInExpression& match) override;

 private:
  void VisitChildren(const plan::Expression& expr);
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
};

class ScanSelectPredicateColumnCollector
    : public plan::ImmutableExpressionVisitor {
 public:
  ScanSelectPredicateColumnCollector() = default;
  virtual ~ScanSelectPredicateColumnCollector() = default;

  std::vector<std::reference_wrapper<const plan::VirtualColumnRefExpression>>
  PredicateColumns();

  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::UnaryArithmeticExpression& arith) override;
  void Visit(const plan::BinaryArithmeticExpression& arith) override;
  void Visit(const plan::CaseExpression& case_expr) override;
  void Visit(const plan::IntToFloatConversionExpression& conv) override;
  void Visit(const plan::ExtractExpression& conv) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::RegexpMatchingExpression& match) override;
  void Visit(const plan::EnumInExpression& match) override;

 private:
  void VisitChildren(const plan::Expression& expr);
  std::vector<std::reference_wrapper<const plan::VirtualColumnRefExpression>>
      predicate_columns_;
};

}  // namespace kush::compile