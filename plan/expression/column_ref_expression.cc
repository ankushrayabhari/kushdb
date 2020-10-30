#include "plan/expression/column_ref_expression.h"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"

namespace kush::plan {

ColumnRefExpression::ColumnRefExpression(int column_idx)
    : column_idx_(column_idx) {}

int ColumnRefExpression::GetColumnIdx() { return column_idx_; }

nlohmann::json ColumnRefExpression::ToJson() const {
  nlohmann::json j;
  j["idx"] = column_idx_;
  return j;
}

}  // namespace kush::plan