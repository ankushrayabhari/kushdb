#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "parse/statement/statement.h"

namespace kush::parse {

std::vector<std::unique_ptr<Statement>> Parse(std::string_view s);

}  // namespace kush::parse