#pragma once

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class VirtualColumnRefExpression : public Expression {
 public:
  VirtualColumnRefExpression(catalog::SqlType type, bool nullable,
                             int column_idx);

  int GetColumnIdx() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  int column_idx_;
};

}  // namespace kush::plan