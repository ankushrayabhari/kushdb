#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class EnumInExpression : public UnaryExpression {
 public:
  EnumInExpression(std::unique_ptr<Expression> child,
                   std::vector<int32_t> values);

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  const std::vector<int>& Values() const;

  nlohmann::json ToJson() const override;

 private:
  std::vector<int32_t> values_;
};

}  // namespace kush::plan