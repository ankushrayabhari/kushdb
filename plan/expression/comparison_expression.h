#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ComparisonType { EQ, NEQ, LT, LEQ, GT, GEQ };

class ComparisonExpression : public BinaryExpression {
 public:
  ComparisonExpression(ComparisonType type, std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);
  nlohmann::json ToJson() const override;
  void Accept(ExpressionVisitor& visitor) override;
  ComparisonType CompType() const;

 private:
  ComparisonType type_;
};

}  // namespace kush::plan