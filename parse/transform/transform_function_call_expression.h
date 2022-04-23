#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformFunctionCallExpression(
    libpgquery::PGFuncCall& func);

}  // namespace kush::parse