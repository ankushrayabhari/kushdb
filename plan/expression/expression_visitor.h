#pragma once

#include <cstdint>

namespace kush::plan {

class ColumnRefExpression;
class ComparisonExpression;

template <typename T>
class LiteralExpression;

class ExpressionVisitor {
 public:
  virtual void Visit(ColumnRefExpression& col_ref) = 0;
  virtual void Visit(ComparisonExpression& comp) = 0;

  // TODO: define this for all runtime types
  virtual void Visit(LiteralExpression<int32_t>& literal) = 0;
};

}  // namespace kush::plan