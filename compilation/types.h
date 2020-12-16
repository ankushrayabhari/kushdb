#pragma once

#include <string>

#include "plan/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(plan::SqlType type);

}  // namespace kush::compile