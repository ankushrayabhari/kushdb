#pragma once

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class ColumnRefExpression : public Expression {
 public:
  ColumnRefExpression(int column_idx);
  ColumnRefExpression(int child_idx, int column_idx);
  nlohmann::json ToJson() const override;
  int GetChildIdx();
  int GetColumnIdx();
  void Accept(ExpressionVisitor& visitor) override;

 private:
  int child_idx_;
  int column_idx_;
};

}  // namespace kush::plan