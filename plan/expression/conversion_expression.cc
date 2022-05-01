#include "plan/expression/conversion_expression.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "magic_enum.hpp"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::Type CalculateConvSqlType(const catalog::Type& child_type) {
  auto id = child_type.type_id;
  if (id != catalog::SqlType::BIGINT && id != catalog::SqlType::INT &&
      id != catalog::SqlType::SMALLINT) {
    throw std::runtime_error("Non-integral child type for conversion.");
  }
  return catalog::Type::Real();
}

IntToFloatConversionExpression::IntToFloatConversionExpression(
    std::unique_ptr<Expression> child)
    : UnaryExpression(CalculateConvSqlType(child->Type()), child->Nullable(),
                      std::move(child)) {}

nlohmann::json IntToFloatConversionExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = this->Type().ToString();
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