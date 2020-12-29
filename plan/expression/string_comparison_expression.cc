
#include "plan/expression/string_comparison_expression.h"

#include <cassert>
#include <memory>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

StringComparisonExpression::StringComparisonExpression(
    StringComparisonType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : BinaryExpression(catalog::SqlType::BOOLEAN, std::move(left),
                       std::move(right)),
      type_(type) {
  assert(LeftChild().Type() == catalog::SqlType::TEXT);
  assert(RightChild().Type() == catalog::SqlType::TEXT);
}

nlohmann::json StringComparisonExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(type_);
  j["left"] = LeftChild().ToJson();
  j["right"] = RightChild().ToJson();
  return j;
}

void StringComparisonExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void StringComparisonExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

StringComparisonType StringComparisonExpression::CompType() const {
  return type_;
}

}  // namespace kush::plan