#include "plan/expression/column_ref_expression.h"

#include <optional>

#include "magic_enum.hpp"

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

ColumnRefExpression::ColumnRefExpression(const catalog::Type& type,
                                         bool nullable, int child_idx,
                                         int column_idx)
    : Expression(type, nullable, {}),
      child_idx_(child_idx),
      column_idx_(column_idx) {}

int ColumnRefExpression::GetChildIdx() const { return child_idx_; }

int ColumnRefExpression::GetColumnIdx() const { return column_idx_; }

nlohmann::json ColumnRefExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = this->Type().ToString();
  j["child_idx"] = child_idx_;
  j["column_idx"] = column_idx_;
  return j;
}

void ColumnRefExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void ColumnRefExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

void ColumnRefExpression::SetColumnIdx(int x) { column_idx_ = x; }

void ColumnRefExpression::SetChildIdx(int x) { child_idx_ = x; }

}  // namespace kush::plan