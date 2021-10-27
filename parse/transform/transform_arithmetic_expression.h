#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformBoolExpression(
    duckdb_libpgquery::PGBoolExpr& expr);

std::unique_ptr<Expression> TransformNullTestExpression(
    duckdb_libpgquery::PGNullTest& expr);

std::unique_ptr<Expression> TransformArithmeticExpression(
    duckdb_libpgquery::PGAExpr& expr);

}  // namespace kush::parse