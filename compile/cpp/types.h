#pragma once

#include <string>

#include "catalog/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(catalog::SqlType type);

}  // namespace kush::compile