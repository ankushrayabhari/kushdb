#pragma once

#include <memory>
#include <vector>

#include "parse/table/table.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Table> TransformFrom(duckdb_libpgquery::PGList& from);

}  // namespace kush::parse