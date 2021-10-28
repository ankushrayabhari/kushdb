#pragma once

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

enum class AggregateType { SUM, AVG, COUNT, MAX, MIN };

class AggregateExpression : public Expression {
 public:
  AggregateExpression(AggregateType type, std::unique_ptr<Expression> child);

  AggregateType Type() const;
  const Expression& Child() const;

 private:
  AggregateType type_;
  std::unique_ptr<Expression> child_;
};

}  // namespace kush::parse