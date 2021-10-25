#pragma once

#include <memory>
#include <vector>

#include "parse/expression/literal_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<LiteralExpression> TransformLiteralExpression(
    duckdb_libpgquery::PGValue value);

}  // namespace kush::parse