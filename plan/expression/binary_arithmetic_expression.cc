
#include "plan/expression/binary_arithmetic_expression.h"

#include <memory>

#include "magic_enum.hpp"

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::SqlType CalculateType(BinaryArithmeticOperatorType type,
                               catalog::SqlType left, catalog::SqlType right) {
  if (left != right) {
    std::runtime_error("Require same type arguments");
  }

  switch (type) {
    case BinaryArithmeticOperatorType::AND:
    case BinaryArithmeticOperatorType::OR:
    case BinaryArithmeticOperatorType::EQ:
    case BinaryArithmeticOperatorType::NEQ:
    case BinaryArithmeticOperatorType::LT:
    case BinaryArithmeticOperatorType::LEQ:
    case BinaryArithmeticOperatorType::GT:
    case BinaryArithmeticOperatorType::GEQ:
    case BinaryArithmeticOperatorType::STARTS_WITH:
    case BinaryArithmeticOperatorType::ENDS_WITH:
    case BinaryArithmeticOperatorType::CONTAINS:
    case BinaryArithmeticOperatorType::LIKE:
      return catalog::SqlType::BOOLEAN;

    default:
      return left;
  }
}

BinaryArithmeticExpression::BinaryArithmeticExpression(
    BinaryArithmeticOperatorType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : BinaryExpression(CalculateType(type, left->Type(), right->Type()),
                       left->Nullable() || right->Nullable(), std::move(left),
                       std::move(right)),
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