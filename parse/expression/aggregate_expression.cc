#include "parse/expression/aggregate_expression.h"

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

AggregateExpression::AggregateExpression(AggregateType type,
                                         std::unique_ptr<Expression> child)
    : type_(type), child_(std::move(child)) {}

}  // namespace kush::parse