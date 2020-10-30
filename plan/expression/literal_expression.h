#pragma once

#include <cstdint>

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

template <typename T>
class LiteralExpression : public Expression {
 public:
  LiteralExpression(const T value);
  T GetValue();
  nlohmann::json ToJson() const override;
  void Accept(ExpressionVisitor& visitor) override;

 private:
  T value_;
};

// TODO: define this for all runtime types
extern template class LiteralExpression<int32_t>;

}  // namespace kush::plan