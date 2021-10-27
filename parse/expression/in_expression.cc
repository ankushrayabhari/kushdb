#include "parse/expression/in_expression.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"

namespace kush::parse {

InExpression::InExpression(std::unique_ptr<Expression> child,
                           std::vector<std::unique_ptr<Expression>> valid)
    : child_(std::move(child)), valid_(std::move(valid)) {}

}  // namespace kush::parse