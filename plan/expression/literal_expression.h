#pragma once

#include <cstdint>

#include "plan/expression/expression.h"

namespace kush::plan {

template <typename T>
class LiteralExpression : public Expression {
  T value_;

 public:
  LiteralExpression(const T value);
  T GetValue();
  nlohmann::json ToJson() const override;
};

// TODO: define this for all runtime types
extern template class LiteralExpression<int32_t>;

}  // namespace kush::plan