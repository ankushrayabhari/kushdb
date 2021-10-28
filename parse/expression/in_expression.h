#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"

namespace kush::parse {

class InExpression : public Expression {
 public:
  InExpression(std::unique_ptr<Expression> child,
               std::vector<std::unique_ptr<Expression>> valid);

  const Expression& Base() const;
  std::vector<std::reference_wrapper<const Expression>> Cases() const;

 private:
  std::unique_ptr<Expression> child_;
  std::vector<std::unique_ptr<Expression>> valid_;
};

}  // namespace kush::parse