
#include "plan/expression/arithmetic_expression.h"

#include <memory>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::SqlType CalculateType(ArithmeticOperatorType type,
                               catalog::SqlType left, catalog::SqlType right) {
  return left;
}

ArithmeticExpression::ArithmeticExpression(ArithmeticOperatorType type,
                                           std::unique_ptr<Expression> left,
                                           std::unique_ptr<Expression> right)
    : BinaryExpression(CalculateType(type, left->Type(), right->Type()),
                       std::move(left), std::move(right)),
      type_(type) {}

nlohmann::json ArithmeticExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(OpType());
  j["left"] = LeftChild().ToJson();
  j["right"] = RightChild().ToJson();
  return j;
}

void ArithmeticExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void ArithmeticExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

ArithmeticOperatorType ArithmeticExpression::OpType() const { return type_; }

}  // namespace kush::plan