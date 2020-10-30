#include "plan/expression/literal_expression.h"

#include <cstdint>

#include "nlohmann/json.hpp"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

template <typename T>
LiteralExpression<T>::LiteralExpression(const T value) : value_(value) {}

template <typename T>
T LiteralExpression<T>::GetValue() {
  return value_;
}

template <typename T>
nlohmann::json LiteralExpression<T>::ToJson() const {
  nlohmann::json j;
  j["value"] = value_;
  return j;
}

template <typename T>
void LiteralExpression<T>::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

// TODO: define this for all runtime types
template class LiteralExpression<int32_t>;

}  // namespace kush::plan
