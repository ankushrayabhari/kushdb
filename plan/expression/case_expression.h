#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class CaseExpression : public Expression {
 public:
  CaseExpression(std::unique_ptr<Expression> cond,
                 std::unique_ptr<Expression> left,
                 std::unique_ptr<Expression> right);

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  const Expression& Cond() const;
  const Expression& Then() const;
  const Expression& Else() const;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan