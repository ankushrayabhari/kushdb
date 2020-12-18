#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ArithmeticOperatorType {
  ADD,
  SUB,
  MUL,
  DIV,
};

class ArithmeticExpression : public BinaryExpression {
 public:
  ArithmeticExpression(ArithmeticOperatorType type,
                       std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);
  nlohmann::json ToJson() const override;
  void Accept(ExpressionVisitor& visitor) override;
  ArithmeticOperatorType OpType() const;

 private:
  ArithmeticOperatorType type_;
};

}  // namespace kush::plan