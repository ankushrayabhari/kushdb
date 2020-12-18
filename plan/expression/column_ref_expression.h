#pragma once

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class ColumnRefExpression : public Expression {
 public:
  ColumnRefExpression(catalog::SqlType type, int child_idx, int column_idx);
  nlohmann::json ToJson() const override;
  int GetChildIdx();
  int GetColumnIdx();
  std::optional<std::pair<int, int>> GetChildIdxColumnIdx();
  void Accept(ExpressionVisitor& visitor) override;

 private:
  int child_idx_;
  int column_idx_;
};

}  // namespace kush::plan