#pragma once

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

enum class UnaryArithmeticExpressionType { NOT, IS_NULL };

class UnaryArithmeticExpression : public Expression {
 public:
  UnaryArithmeticExpression(UnaryArithmeticExpressionType type,
                            std::unique_ptr<Expression> child);

  UnaryArithmeticExpressionType Type() const;
  const Expression& Child() const;

 private:
  UnaryArithmeticExpressionType type_;
  std::unique_ptr<Expression> child_;
};

enum class BinaryArithmeticExpressionType { AND, OR };

class BinaryArithmeticExpression : public Expression {
 public:
  BinaryArithmeticExpression(BinaryArithmeticExpressionType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

  BinaryArithmeticExpressionType Type() const;
  const Expression& LeftChild() const;
  const Expression& RightChild() const;

 private:
  BinaryArithmeticExpressionType type_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

}  // namespace kush::parse