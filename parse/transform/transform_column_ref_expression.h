#pragma once

#include <memory>
#include <vector>

#include "parse/expression/column_ref_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<ColumnRefExpression> TransformColumnRefExpression(
    duckdb_libpgquery::PGColumnRef& colref);

}  // namespace kush::parse