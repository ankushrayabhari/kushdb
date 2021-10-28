#include "parse/expression/in_expression.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "util/vector_util.h"

namespace kush::parse {

InExpression::InExpression(std::unique_ptr<Expression> child,
                           std::vector<std::unique_ptr<Expression>> valid)
    : child_(std::move(child)), valid_(std::move(valid)) {}

const Expression& InExpression::Base() const { return *child_; }

std::vector<std::reference_wrapper<const Expression>> InExpression::Cases()
    const {
  return util::ImmutableReferenceVector(valid_);
}

}  // namespace kush::parse