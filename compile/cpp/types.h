#pragma once

#include <string>

#include "catalog/sql_type.h"

namespace kush::compile::cpp {

std::string SqlTypeToRuntimeType(catalog::SqlType type);

}  // namespace kush::compile::cpp