#pragma once

#include <optional>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class AggregateType { SUM, AVG };

class AggregateExpression : public UnaryExpression {
 public:
  AggregateExpression(AggregateType type, std::unique_ptr<Expression> child);
  nlohmann::json ToJson() const override;
  AggregateType AggType();
  void Accept(ExpressionVisitor& visitor) override;

 private:
  AggregateType type_;
};

}  // namespace kush::plan