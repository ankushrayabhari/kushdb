#pragma once

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

enum class UnaryArithmeticExpressionType { NOT, IS_NULL };

class UnaryArithmeticExpression : public Expression {
 public:
  UnaryArithmeticExpression(UnaryArithmeticExpressionType type,
                            std::unique_ptr<Expression> child);

 private:
  UnaryArithmeticExpressionType type_;
  std::unique_ptr<Expression> child_;
};

enum class BinaryArithmeticExpressionType { PLUS, MINUS, TIMES, AND, OR };

class BinaryArithmeticExpression : public Expression {
 public:
  BinaryArithmeticExpression(BinaryArithmeticExpressionType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

 private:
  BinaryArithmeticExpressionType type_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

}  // namespace kush::parse