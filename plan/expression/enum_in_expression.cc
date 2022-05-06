
#include "plan/expression/enum_in_expression.h"

#include <memory>

#include "magic_enum.hpp"

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::Type CalculateEnumInType(const catalog::Type& child_type) {
  if (child_type.type_id != catalog::TypeId::ENUM) {
    throw std::runtime_error("Invalid child type for Enum in. Expected enum.");
  }
  return catalog::Type::Boolean();
}

EnumInExpression::EnumInExpression(std::unique_ptr<Expression> child,
                                   std::vector<int> values)
    : UnaryExpression(CalculateEnumInType(child->Type()), false,
                      std::move(child)),
      values_(std::move(values)) {}

void EnumInExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void EnumInExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

nlohmann::json EnumInExpression::ToJson() const {
  nlohmann::json j;
  j["child"] = Child().ToJson();
  for (int v : values_) {
    j["values"].push_back(v);
  }
  return j;
}

const std::vector<int>& EnumInExpression::Values() const { return values_; }

}  // namespace kush::plan