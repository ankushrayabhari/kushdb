#include "parse/expression/arithmetic_expression.h"

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

UnaryArithmeticExpression::UnaryArithmeticExpression(
    UnaryArithmeticExpressionType type, std::unique_ptr<Expression> child)
    : type_(type), child_(std::move(child)) {}

UnaryArithmeticExpressionType UnaryArithmeticExpression::Type() const {
  return type_;
}

const Expression& UnaryArithmeticExpression::Child() const { return *child_; }

BinaryArithmeticExpression::BinaryArithmeticExpression(
    BinaryArithmeticExpressionType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : type_(type), left_(std::move(left)), right_(std::move(right)) {}

BinaryArithmeticExpressionType BinaryArithmeticExpression::Type() const {
  return type_;
}

const Expression& BinaryArithmeticExpression::LeftChild() const {
  return *left_;
}

const Expression& BinaryArithmeticExpression::RightChild() const {
  return *right_;
}

}  // namespace kush::parse