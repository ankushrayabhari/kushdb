#pragma once

#include <memory>

#include "re2/re2.h"

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class UnaryArithmeticExpressionType {
  NOT,
  IS_NULL,
};

class UnaryArithmeticExpression : public UnaryExpression {
 public:
  UnaryArithmeticExpression(UnaryArithmeticExpressionType type,
                            std::unique_ptr<Expression> child);

  UnaryArithmeticExpressionType OpType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  UnaryArithmeticExpressionType type_;
};

enum class BinaryArithmeticExpressionType {
  ADD,
  SUB,
  MUL,
  DIV,
  EQ,
  NEQ,
  LT,
  LEQ,
  GT,
  GEQ,
  AND,
  OR,
  STARTS_WITH,
  ENDS_WITH,
  CONTAINS
};

class BinaryArithmeticExpression : public BinaryExpression {
 public:
  BinaryArithmeticExpression(BinaryArithmeticExpressionType type,
                             std::unique_ptr<Expression> left,
                             std::unique_ptr<Expression> right);

  BinaryArithmeticExpressionType OpType() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  BinaryArithmeticExpressionType type_;
};

class RegexpMatchingExpression : public UnaryExpression {
 public:
  RegexpMatchingExpression(std::unique_ptr<Expression> child,
                           std::unique_ptr<re2::RE2> regexp);

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  re2::RE2* Regex() const;

  nlohmann::json ToJson() const override;

 private:
  std::unique_ptr<re2::RE2> regexp_;
};

}  // namespace kush::plan