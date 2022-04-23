#pragma once

#include <memory>

#include "parse/statement/select_statement.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<SelectStatement> TransformSelectStatement(
    libpgquery::PGNode& stmt);

}  // namespace kush::parse