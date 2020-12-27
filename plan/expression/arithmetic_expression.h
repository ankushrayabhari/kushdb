#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ArithmeticOperatorType {
  ADD,
  SUB,
  MUL,
  DIV,
};

class ArithmeticExpression : public BinaryExpression {
 public:
  ArithmeticExpression(ArithmeticOperatorType type,
                       std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);

  ArithmeticOperatorType OpType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  ArithmeticOperatorType type_;
};

}  // namespace kush::plan