#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformExpression(
    duckdb_libpgquery::PGNode& expr);

std::vector<std::unique_ptr<Expression>> TransformExpressionList(
    duckdb_libpgquery::PGList& list);

}  // namespace kush::parse