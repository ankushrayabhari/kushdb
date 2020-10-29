#include "plan/expression/literal_expression.h"

#include <cstdint>

namespace kush::plan {

template <typename T>
LiteralExpression<T>::LiteralExpression(const T value) : value_(value) {}

// TODO: define this for all runtime types
template class LiteralExpression<int64_t>;

}  // namespace kush::plan
