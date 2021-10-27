#include "parse/expression/arithmetic_expression.h"

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

UnaryArithmeticExpression::UnaryArithmeticExpression(
    UnaryArithmeticExpressionType type, std::unique_ptr<Expression> child)
    : type_(type), child_(std::move(child)) {}

BinaryArithmeticExpression::BinaryArithmeticExpression(
    BinaryArithmeticExpressionType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : type_(type), left_(std::move(left)), right_(std::move(right)) {}

}  // namespace kush::parse