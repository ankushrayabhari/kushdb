#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ComparisonType { EQ, NEQ, LT, LEQ, GT, GEQ };

class ComparisonExpression : public Expression {
 public:
  ComparisonExpression(ComparisonType type, std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);
  nlohmann::json ToJson() const override;
  void Accept(ExpressionVisitor& visitor) override;

 private:
  ComparisonType type_;
  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> right_;
};

}  // namespace kush::plan