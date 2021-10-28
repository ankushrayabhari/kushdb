#include "parse/expression/aggregate_expression.h"

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

AggregateExpression::AggregateExpression(AggregateType type,
                                         std::unique_ptr<Expression> child)
    : type_(type), child_(std::move(child)) {}

AggregateType AggregateExpression::Type() const { return type_; }

const Expression& AggregateExpression::Child() const { return *child_; }

}  // namespace kush::parse