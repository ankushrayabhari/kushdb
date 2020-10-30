#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class Expression {
 public:
  virtual ~Expression() = default;
  virtual nlohmann::json ToJson() const = 0;
  virtual void Accept(ExpressionVisitor& visitor) = 0;
};

}  // namespace kush::plan