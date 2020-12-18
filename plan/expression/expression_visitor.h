#pragma once

#include <cstdint>

namespace kush::plan {

class AggregateExpression;
class ColumnRefExpression;
class ComparisonExpression;
class LiteralExpression;
class VirtualColumnRefExpression;
class ArithmeticExpression;

class ExpressionVisitor {
 public:
  virtual void Visit(ArithmeticExpression& arith) = 0;
  virtual void Visit(AggregateExpression& agg) = 0;
  virtual void Visit(ColumnRefExpression& col_ref) = 0;
  virtual void Visit(VirtualColumnRefExpression& virtual_col_ref) = 0;
  virtual void Visit(ComparisonExpression& comp) = 0;
  virtual void Visit(LiteralExpression& literal) = 0;
};

}  // namespace kush::plan