#pragma once

#include <optional>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class AggregateType { SUM, AVG };

class AggregateExpression : public Expression {
 public:
  AggregateExpression(AggregateType type, std::unique_ptr<Expression> child);
  nlohmann::json ToJson() const override;
  AggregateType Type();
  Expression& Child();
  void Accept(ExpressionVisitor& visitor) override;

 private:
  std::unique_ptr<Expression> child_;
  AggregateType type_;
};

}  // namespace kush::plan