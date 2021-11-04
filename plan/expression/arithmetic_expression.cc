
#include "plan/expression/arithmetic_expression.h"

#include <memory>

#include "magic_enum.hpp"

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::SqlType CalculateBinaryType(BinaryArithmeticExpressionType type,
                                     catalog::SqlType left,
                                     catalog::SqlType right) {
  if (left != right) {
    std::runtime_error("Require same type arguments");
  }

  switch (type) {
    case BinaryArithmeticExpressionType::AND:
    case BinaryArithmeticExpressionType::OR:
    case BinaryArithmeticExpressionType::EQ:
    case BinaryArithmeticExpressionType::NEQ:
    case BinaryArithmeticExpressionType::LT:
    case BinaryArithmeticExpressionType::LEQ:
    case BinaryArithmeticExpressionType::GT:
    case BinaryArithmeticExpressionType::GEQ:
    case BinaryArithmeticExpressionType::STARTS_WITH:
    case BinaryArithmeticExpressionType::ENDS_WITH:
    case BinaryArithmeticExpressionType::CONTAINS:
    case BinaryArithmeticExpressionType::LIKE:
      return catalog::SqlType::BOOLEAN;

    default:
      return left;
  }
}

BinaryArithmeticExpression::BinaryArithmeticExpression(
    BinaryArithmeticExpressionType type, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right)
    : BinaryExpression(CalculateBinaryType(type, left->Type(), right->Type()),
                       left->Nullable() || right->Nullable(), std::move(left),
                       std::move(right)),
      type_(type) {}

nlohmann::json BinaryArithmeticExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = magic_enum::enum_name(this->Type());
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

BinaryArithmeticExpressionType BinaryArithmeticExpression::OpType() const {
  return type_;
}

catalog::SqlType CalculateUnaryType(UnaryArithmeticExpressionType type,
                                    catalog::SqlType child_type) {
  switch (type) {
    case UnaryArithmeticExpressionType::NOT: {
      if (child_type != catalog::SqlType::BOOLEAN) {
        throw std::runtime_error("Not boolean child");
      }
      return catalog::SqlType::BOOLEAN;
    }

    case UnaryArithmeticExpressionType::IS_NULL:
      return catalog::SqlType::BOOLEAN;
  }
}

bool CalculateNullability(UnaryArithmeticExpressionType type,
                          bool child_nullability) {
  switch (type) {
    case UnaryArithmeticExpressionType::IS_NULL:
      return false;

    case UnaryArithmeticExpressionType::NOT:
      return child_nullability;
  }
}

UnaryArithmeticExpression::UnaryArithmeticExpression(
    UnaryArithmeticExpressionType type, std::unique_ptr<Expression> child)
    : UnaryExpression(CalculateUnaryType(type, child->Type()),
                      CalculateNullability(type, child->Nullable()),
                      std::move(child)),
      type_(type) {}

UnaryArithmeticExpressionType UnaryArithmeticExpression::OpType() const {
  return type_;
}

void UnaryArithmeticExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void UnaryArithmeticExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

nlohmann::json UnaryArithmeticExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(OpType());
  j["child"] = Child().ToJson();
  return j;
}

}  // namespace kush::plan