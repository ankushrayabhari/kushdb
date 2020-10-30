#pragma once

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"

namespace kush::plan {

class ColumnRefExpression : public Expression {
 public:
  ColumnRefExpression(int column_idx);
  nlohmann::json ToJson() const override;
  int GetColumnIdx();

 private:
  int column_idx_;
};

}  // namespace kush::plan