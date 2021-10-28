#pragma once

#include <vector>

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

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

 private:
  void VisitChildren(const plan::Expression& expr);
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
};

}  // namespace kush::compile