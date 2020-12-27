#pragma once

#include <optional>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class AggregateType { SUM, AVG, COUNT };

class AggregateExpression : public UnaryExpression {
 public:
  AggregateExpression(AggregateType type, std::unique_ptr<Expression> child);

  AggregateType AggType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  AggregateType type_;
};

}  // namespace kush::plan