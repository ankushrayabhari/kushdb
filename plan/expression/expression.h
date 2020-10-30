#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

namespace kush::plan {

class Expression {
 public:
  virtual ~Expression() = default;
  virtual nlohmann::json ToJson() const = 0;
};

}  // namespace kush::plan