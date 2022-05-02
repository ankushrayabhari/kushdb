
#include "plan/expression/arithmetic_expression.h"

#include <memory>

#include "magic_enum.hpp"

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

catalog::Type CalculateBinaryType(BinaryArithmeticExpressionType type,
                                  const catalog::Type& left,
                                  const catalog::Type& right) {
  if (left.type_id == catalog::TypeId::ENUM) {
    switch (type) {
      case BinaryArithmeticExpressionType::EQ:
      case BinaryArithmeticExpressionType::NEQ: {
        if (left != right) {
          throw std::runtime_error("Require same type arguments");
        }
        return catalog::Type::Boolean();
      }

      case BinaryArithmeticExpressionType::LT:
      case BinaryArithmeticExpressionType::LEQ:
      case BinaryArithmeticExpressionType::GT:
      case BinaryArithmeticExpressionType::GEQ: {
        if (right.type_id != catalog::TypeId::TEXT && left != right) {
          throw std::runtime_error("Require same type arguments");
        }
        return catalog::Type::Boolean();
      }

      case BinaryArithmeticExpressionType::STARTS_WITH:
      case BinaryArithmeticExpressionType::ENDS_WITH:
      case BinaryArithmeticExpressionType::CONTAINS: {
        if (right.type_id != catalog::TypeId::TEXT && left != right) {
          throw std::runtime_error("Require same type arguments");
        }
        return catalog::Type::Boolean();
      }

      default:
        throw std::runtime_error("Invalid expr type");
    }
  }

  if (right.type_id == catalog::TypeId::ENUM) {
    throw std::runtime_error("not supported yet");
  }

  if (left != right) {
    throw std::runtime_error("Require same type arguments");
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
      return catalog::Type::Boolean();

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
  j["type"] = this->Type().ToString();
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

catalog::Type CalculateUnaryType(UnaryArithmeticExpressionType type,
                                 const catalog::Type& child_type) {
  switch (type) {
    case UnaryArithmeticExpressionType::NOT: {
      if (child_type.type_id != catalog::TypeId::BOOLEAN) {
        throw std::runtime_error("Not boolean child");
      }
      return catalog::Type::Boolean();
    }

    case UnaryArithmeticExpressionType::IS_NULL:
      return catalog::Type::Boolean();
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

catalog::Type CalculateRegexpType(const catalog::Type& child_type) {
  if (child_type.type_id != catalog::TypeId::TEXT &&
      child_type.type_id != catalog::TypeId::ENUM) {
    throw std::runtime_error(
        "Invalid child type for LIKE. Expected text/enum.");
  }
  return catalog::Type::Boolean();
}

RegexpMatchingExpression::RegexpMatchingExpression(
    std::unique_ptr<Expression> child, std::unique_ptr<re2::RE2> regexp)
    : UnaryExpression(CalculateRegexpType(child->Type()), false,
                      std::move(child)),
      regexp_(std::move(regexp)) {}

void RegexpMatchingExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void RegexpMatchingExpression::Accept(
    ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

nlohmann::json RegexpMatchingExpression::ToJson() const {
  nlohmann::json j;
  j["child"] = Child().ToJson();
  j["regex"] = regexp_->pattern();
  return j;
}

re2::RE2* RegexpMatchingExpression::Regex() const { return regexp_.get(); }

}  // namespace kush::plan