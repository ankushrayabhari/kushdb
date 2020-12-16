#pragma once

#include <cstdint>

namespace kush::plan {

class ColumnRefExpression;
class ComparisonExpression;
class LiteralExpression;

class ExpressionVisitor {
 public:
  virtual void Visit(ColumnRefExpression& col_ref) = 0;
  virtual void Visit(ComparisonExpression& comp) = 0;
  virtual void Visit(LiteralExpression& literal) = 0;
};

}  // namespace kush::plan