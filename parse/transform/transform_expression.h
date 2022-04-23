#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformExpression(libpgquery::PGNode& expr);

std::vector<std::unique_ptr<Expression>> TransformExpressionList(
    libpgquery::PGList& list);

}  // namespace kush::parse