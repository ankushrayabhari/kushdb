#include "plan/expression/column_ref_expression.h"

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

ColumnRefExpression::ColumnRefExpression(catalog::SqlType type, int child_idx,
                                         int column_idx)
    : Expression(type, {}), child_idx_(child_idx), column_idx_(column_idx) {}

int ColumnRefExpression::GetChildIdx() { return child_idx_; }

int ColumnRefExpression::GetColumnIdx() { return column_idx_; }

nlohmann::json ColumnRefExpression::ToJson() const {
  nlohmann::json j;
  j["child_idx"] = child_idx_;
  j["column_idx"] = column_idx_;
  return j;
}

void ColumnRefExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

}  // namespace kush::plan