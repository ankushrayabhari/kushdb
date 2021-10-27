#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformFunctionCallExpression(
    duckdb_libpgquery::PGFuncCall& func);

}  // namespace kush::parse