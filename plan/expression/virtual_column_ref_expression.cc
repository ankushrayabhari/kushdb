#include "plan/expression/virtual_column_ref_expression.h"

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

VirtualColumnRefExpression::VirtualColumnRefExpression(catalog::SqlType type,
                                                       bool nullable,
                                                       int column_idx)
    : Expression(type, nullable, {}), column_idx_(column_idx) {}

int VirtualColumnRefExpression::GetColumnIdx() const { return column_idx_; }

nlohmann::json VirtualColumnRefExpression::ToJson() const {
  nlohmann::json j;
  j["column_idx"] = column_idx_;
  return j;
}

void VirtualColumnRefExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void VirtualColumnRefExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan