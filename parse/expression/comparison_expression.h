#pragma once

#include <memory>

#include "parse/expression/expression.h"

namespace kush::parse {

enum ComparisonType { EQ, NEQ, LT, LEQ, GT, GEQ, LIKE };

class ComparisonExpression : public Expression {
 public:
  ComparisonExpression(ComparisonType type, std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);

 private:
  ComparisonType type_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

}  // namespace kush::parse