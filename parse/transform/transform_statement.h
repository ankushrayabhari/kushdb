#pragma once

#include <memory>

#include "parse/statement/statement.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Statement> TransformStatement(libpgquery::PGNode& stmt);

}  // namespace kush::parse