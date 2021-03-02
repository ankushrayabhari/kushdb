#pragma once

#include <optional>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class IntToFloatConversionExpression : public UnaryExpression {
 public:
  IntToFloatConversionExpression(std::unique_ptr<Expression> child);

  catalog::SqlType Type() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan