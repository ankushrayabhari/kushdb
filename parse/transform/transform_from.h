#pragma once

#include <memory>
#include <vector>

#include "parse/table/table.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Table> TransformFrom(libpgquery::PGList& from);

}  // namespace kush::parse