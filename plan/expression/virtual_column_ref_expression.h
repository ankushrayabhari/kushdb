#pragma once

#include <optional>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class VirtualColumnRefExpression : public Expression {
 public:
  VirtualColumnRefExpression(catalog::SqlType type, int column_idx);
  nlohmann::json ToJson() const override;
  int GetColumnIdx();
  void Accept(ExpressionVisitor& visitor) override;

 private:
  int column_idx_;
};

}  // namespace kush::plan