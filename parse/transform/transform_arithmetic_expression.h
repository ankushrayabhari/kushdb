#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformBoolExpression(
    libpgquery::PGBoolExpr& expr);

std::unique_ptr<Expression> TransformNullTestExpression(
    libpgquery::PGNullTest& expr);

std::unique_ptr<Expression> TransformArithmeticExpression(
    libpgquery::PGAExpr& expr);

}  // namespace kush::parse