#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::string TransformAlias(duckdb_libpgquery::PGAlias* root);

}  // namespace kush::parse