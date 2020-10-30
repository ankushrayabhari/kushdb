#pragma once

#include <cstdint>

namespace kush::plan {

class ColumnRefExpression;
class ComparisonExpression;

template <typename T>
class LiteralExpression;

class ExpressionVisitor {
 public:
  virtual void Visit(ColumnRefExpression& scan) = 0;
  virtual void Visit(ComparisonExpression& select) = 0;

  // TODO: define this for all runtime types
  virtual void Visit(LiteralExpression<int32_t>& output) = 0;
};

}  // namespace kush::plan