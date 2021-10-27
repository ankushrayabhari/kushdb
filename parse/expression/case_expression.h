#pragma once

#include <memory>
#include <vector>

#include "parse/expression/expression.h"

namespace kush::parse {

class CaseExpression : public Expression {
 public:
  CaseExpression(std::vector<std::pair<std::unique_ptr<Expression>,
                                       std::unique_ptr<Expression>>>
                     cases,
                 std::unique_ptr<Expression> fallthrough);

 private:
  std::vector<
      std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>
      cases_;
  std::unique_ptr<Expression> fallthrough_;
};

}  // namespace kush::parse