#pragma once

#include <memory>

#include "parse/statement/select_statement.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<SelectStatement> TransformSelectStatement(
    duckdb_libpgquery::PGNode& stmt);

}  // namespace kush::parse