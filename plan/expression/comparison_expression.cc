
#include "plan/expression/comparison_expression.h"

#include <memory>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

ComparisonExpression::ComparisonExpression(ComparisonType type,
                                           std::unique_ptr<Expression> left,
                                           std::unique_ptr<Expression> right)
    : BinaryExpression(catalog::SqlType::BOOLEAN, std::move(left),
                       std::move(right)),
      type_(type) {}

nlohmann::json ComparisonExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(ComparisonType());
  j["left"] = LeftChild().ToJson();
  j["right"] = RightChild().ToJson();
  return j;
}

void ComparisonExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void ComparisonExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

ComparisonType ComparisonExpression::CompType() const { return type_; }

}  // namespace kush::plan