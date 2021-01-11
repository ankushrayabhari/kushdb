
#include "plan/expression/binary_arithmetic_expression.h"

#include <memory>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::SqlType CalculateType(BinaryArithmeticOperatorType type,
                               catalog::SqlType left, catalog::SqlType right) {
  // TODO: calculate type
  return left;
}

BinaryArithmeticExpression::BinaryArithmeticExpression(
    BinaryArithmeticOperatorType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : BinaryExpression(CalculateType(type, left->Type(), right->Type()),
                       std::move(left), std::move(right)),
      type_(type) {}

nlohmann::json BinaryArithmeticExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(OpType());
  j["left"] = LeftChild().ToJson();
  j["right"] = RightChild().ToJson();
  return j;
}

void BinaryArithmeticExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void BinaryArithmeticExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

BinaryArithmeticOperatorType BinaryArithmeticExpression::OpType() const {
  return type_;
}

}  // namespace kush::plan