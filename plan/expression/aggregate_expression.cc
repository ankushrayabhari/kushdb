#include "plan/expression/aggregate_expression.h"

#include <memory>

#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

AggregateExpression::AggregateExpression(AggregateType type,
                                         std::unique_ptr<Expression> child)
    : child_(std::move(child)), type_(type) {}

nlohmann::json AggregateExpression::ToJson() const {
  nlohmann::json j;
  j["agg"] = magic_enum::enum_name(type_);
  j["child"] = child_->ToJson();
  return j;
}

void AggregateExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

Expression& AggregateExpression::Child() { return *child_; }

AggregateType AggregateExpression::Type() { return type_; }

}  // namespace kush::plan