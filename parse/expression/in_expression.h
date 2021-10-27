#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"

namespace kush::parse {

class InExpression : public Expression {
 public:
  InExpression(std::unique_ptr<Expression> child,
               std::vector<std::unique_ptr<Expression>> valid);

 private:
  std::unique_ptr<Expression> child_;
  std::vector<std::unique_ptr<Expression>> valid_;
};

}  // namespace kush::parse