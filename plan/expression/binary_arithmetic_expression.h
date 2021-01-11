#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class BinaryArithmeticOperatorType {
  ADD,
  SUB,
  MUL,
  DIV,
  EQ,
  NEQ,
  LT,
  LEQ,
  GT,
  GEQ,
  AND,
  OR,
  STARTS_WITH,
  ENDS_WITH,
  CONTAINS
};

class BinaryArithmeticExpression : public BinaryExpression {
 public:
  BinaryArithmeticExpression(BinaryArithmeticOperatorType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

  BinaryArithmeticOperatorType OpType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  BinaryArithmeticOperatorType type_;
};

}  // namespace kush::plan