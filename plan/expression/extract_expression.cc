#include "plan/expression/extract_expression.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "magic_enum.hpp"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

using SqlType = catalog::SqlType;

SqlType CalculateExtractSqlType(catalog::SqlType child_type) {
  if (child_type != SqlType::DATE) {
    throw std::runtime_error("Non-date child type for extract.");
  }
  return SqlType::INT;
}

ExtractExpression::ExtractExpression(std::unique_ptr<Expression> child,
                                     ExtractValue value)
    : UnaryExpression(CalculateExtractSqlType(child->Type()), child->Nullable(),
                      std::move(child)),
      value_to_extract_(value) {}

ExtractValue ExtractExpression::ValueToExtract() const {
  return value_to_extract_;
}

nlohmann::json ExtractExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = magic_enum::enum_name(this->Type());
  j["extract"] = magic_enum::enum_name(value_to_extract_);
  j["child"] = Child().ToJson();
  return j;
}

void ExtractExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void ExtractExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan