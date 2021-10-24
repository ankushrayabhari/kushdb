#pragma once

#include <memory>

#include "parse/statement/statement.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Statement> TransformStatement(duckdb_libpgquery::PGNode& stmt);

}  // namespace kush::parse