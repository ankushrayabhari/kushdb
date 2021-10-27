#pragma once

#include <memory>
#include <vector>

#include "parse/expression/case_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<CaseExpression> TransformCaseExpression(
    duckdb_libpgquery::PGCaseExpr& expr);

}  // namespace kush::parse