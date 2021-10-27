#include "parse/expression/comparison_expression.h"

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

ComparisonExpression::ComparisonExpression(ComparisonType type,
                                           std::unique_ptr<Expression> left,
                                           std::unique_ptr<Expression> right)
    : type_(type), left_(std::move(left)), right_(std::move(right)) {}

}  // namespace kush::parse