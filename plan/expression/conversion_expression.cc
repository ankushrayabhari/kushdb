#include "plan/expression/conversion_expression.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "magic_enum.hpp"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

using SqlType = catalog::SqlType;

SqlType CalculateConvSqlType(catalog::SqlType child_type) {
  if (child_type != SqlType::BIGINT && child_type != SqlType::INT &&
      child_type != SqlType::SMALLINT) {
    throw std::runtime_error("Non-integral child type for conversion.");
  }
  return SqlType::REAL;
}

IntToFloatConversionExpression::IntToFloatConversionExpression(
    std::unique_ptr<Expression> child)
    : UnaryExpression(CalculateConvSqlType(child->Type()), std::move(child)) {}

nlohmann::json IntToFloatConversionExpression::ToJson() const {
  nlohmann::json j;
  j["child"] = Child().ToJson();
  return j;
}

void IntToFloatConversionExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void IntToFloatConversionExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan