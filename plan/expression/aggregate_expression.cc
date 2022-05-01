#include "plan/expression/aggregate_expression.h"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "magic_enum.hpp"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

using SqlType = catalog::SqlType;

catalog::Type CalculateAggSqlType(AggregateType type, const Expression& expr) {
  auto child_type = expr.Type();
  auto child_type_id = child_type.type_id;
  switch (type) {
    case AggregateType::SUM:
      if (child_type_id == SqlType::INT || child_type_id == SqlType::SMALLINT ||
          child_type_id == SqlType::BIGINT || child_type_id == SqlType::REAL) {
        return child_type;
      } else {
        throw std::runtime_error("Invalid input type for sum aggregate");
      }

    case AggregateType::AVG:
      if (child_type_id == SqlType::INT || child_type_id == SqlType::SMALLINT ||
          child_type_id == SqlType::BIGINT || child_type_id == SqlType::REAL) {
        return catalog::Type::Real();
      } else {
        throw std::runtime_error("Invalid input type for avg aggregate");
      }

    case AggregateType::COUNT:
      return catalog::Type::BigInt();

    case AggregateType::MIN:
    case AggregateType::MAX:
      if (child_type_id == SqlType::INT || child_type_id == SqlType::SMALLINT ||
          child_type_id == SqlType::BIGINT || child_type_id == SqlType::REAL ||
          child_type_id == SqlType::DATE || child_type_id == SqlType::TEXT) {
        return child_type;
      } else {
        throw std::runtime_error("Invalid input type for max/min aggregate");
      }
  }
}

AggregateExpression::AggregateExpression(AggregateType type,
                                         std::unique_ptr<Expression> child)
    : UnaryExpression(CalculateAggSqlType(type, *child),
                      type != AggregateType::COUNT, std::move(child)),
      type_(type) {}

nlohmann::json AggregateExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = this->Type().ToString();
  j["agg_type"] = magic_enum::enum_name(type_);
  j["child"] = Child().ToJson();
  return j;
}

void AggregateExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void AggregateExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

AggregateType AggregateExpression::AggType() const { return type_; }

}  // namespace kush::plan