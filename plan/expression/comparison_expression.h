#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ComparisonType { EQ, NEQ, LT, LEQ, GT, GEQ, AND, OR };

class ComparisonExpression : public BinaryExpression {
 public:
  ComparisonExpression(ComparisonType type, std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);

  ComparisonType CompType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  ComparisonType type_;
};

}  // namespace kush::plan