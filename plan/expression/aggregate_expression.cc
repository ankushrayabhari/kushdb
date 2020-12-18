#include "plan/expression/aggregate_expression.h"

#include <memory>
#include <stdexcept>

#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

using SqlType = catalog::SqlType;

SqlType CalculateSqlType(AggregateType type, const Expression& expr) {
  auto child_type = expr.Type();
  switch (type) {
    case AggregateType::SUM:
      if (child_type == SqlType::INT || child_type == SqlType::SMALLINT ||
          child_type == SqlType::BIGINT) {
        return child_type;
      } else {
        throw std::runtime_error("Invalid input type for sum aggregate");
      }
      break;

    case AggregateType::AVG:
      if (child_type == SqlType::INT || child_type == SqlType::SMALLINT ||
          child_type == SqlType::BIGINT || child_type == SqlType::REAL) {
        return SqlType::REAL;
      } else {
        throw std::runtime_error("Invalid input type for sum aggregate");
      }
      break;

    case AggregateType::COUNT:
      return SqlType::BIGINT;
  }
}

AggregateExpression::AggregateExpression(AggregateType type,
                                         std::unique_ptr<Expression> child)
    : UnaryExpression(CalculateSqlType(type, *child), std::move(child)),
      type_(type) {}

nlohmann::json AggregateExpression::ToJson() const {
  nlohmann::json j;
  j["agg"] = magic_enum::enum_name(type_);
  j["child"] = Child().ToJson();
  return j;
}

void AggregateExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

AggregateType AggregateExpression::AggType() { return type_; }

}  // namespace kush::plan