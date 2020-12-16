#include "plan/expression/literal_expression.h"

#include <cstdint>

#include "nlohmann/json.hpp"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

LiteralExpression::LiteralExpression(int32_t value) : value_(value) {}

int32_t LiteralExpression::GetValue() { return value_; }

nlohmann::json LiteralExpression::ToJson() const {
  nlohmann::json j;
  j["value"] = value_;
  return j;
}

void LiteralExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

}  // namespace kush::plan
