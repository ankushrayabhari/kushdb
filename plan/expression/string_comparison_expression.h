#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class StringComparisonType { STARTS_WITH, ENDS_WITH, CONTAINS };

class StringComparisonExpression : public BinaryExpression {
 public:
  StringComparisonExpression(StringComparisonType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

  StringComparisonType CompType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  StringComparisonType type_;
};

}  // namespace kush::plan