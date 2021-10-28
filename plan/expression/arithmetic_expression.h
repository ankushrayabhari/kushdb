#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class BinaryArithmeticExpressionType {
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
  CONTAINS,
  LIKE
};

class BinaryArithmeticExpression : public BinaryExpression {
 public:
  BinaryArithmeticExpression(BinaryArithmeticExpressionType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

  BinaryArithmeticExpressionType OpType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  BinaryArithmeticExpressionType type_;
};

}  // namespace kush::plan