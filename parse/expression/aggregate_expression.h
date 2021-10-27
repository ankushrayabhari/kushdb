#pragma once

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

enum class AggregateType { COUNT, MIN, MAX, SUM, AVG };

class AggregateExpression : public Expression {
 public:
  AggregateExpression(AggregateType type, std::unique_ptr<Expression> child);

 private:
  AggregateType type_;
  std::unique_ptr<Expression> child_;
};

}  // namespace kush::parse