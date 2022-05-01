#pragma once

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class ColumnRefExpression : public Expression {
 public:
  ColumnRefExpression(const catalog::Type& type, bool nullable, int child_idx,
                      int column_idx);

  int GetChildIdx() const;
  int GetColumnIdx() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

  void SetChildIdx(int x);
  void SetColumnIdx(int x);

 private:
  int child_idx_;
  int column_idx_;
};

}  // namespace kush::plan