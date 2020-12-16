#pragma once

#include <cstdint>

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class LiteralExpression : public Expression {
 public:
  LiteralExpression(int32_t value);
  int32_t GetValue();
  nlohmann::json ToJson() const override;
  void Accept(ExpressionVisitor& visitor) override;

 private:
  int32_t value_;
};

}  // namespace kush::plan