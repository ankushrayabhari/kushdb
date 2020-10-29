#pragma once

#include <cstdint>

namespace kush::plan {

template <typename T>
class LiteralExpression {
  T value_;

 public:
  LiteralExpression(const T value);
};

// TODO: define this for all runtime types
extern template class LiteralExpression<int64_t>;

}  // namespace kush::plan