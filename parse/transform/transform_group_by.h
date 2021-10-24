#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::vector<std::unique_ptr<Expression>> TransformGroupBy(
    duckdb_libpgquery::PGList& group_by);

}  // namespace kush::parse