#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/select_statement.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> TransformOrderBy(
    duckdb_libpgquery::PGList& group_by);

}  // namespace kush::parse