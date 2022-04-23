#pragma once

#include <memory>
#include <vector>

#include "parse/expression/literal_expression.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<LiteralExpression> TransformLiteralExpression(
    libpgquery::PGValue value);

}  // namespace kush::parse