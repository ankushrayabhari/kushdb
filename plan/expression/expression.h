#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class Expression {
 public:
  Expression(catalog::SqlType type, bool nullable,
             std::vector<std::unique_ptr<Expression>> children);
  virtual ~Expression() = default;

  std::vector<std::reference_wrapper<const Expression>> Children() const;
  std::vector<std::reference_wrapper<Expression>> MutableChildren();
  catalog::SqlType Type() const;
  bool Nullable() const;

  virtual void Accept(ExpressionVisitor& visitor) = 0;
  virtual void Accept(ImmutableExpressionVisitor& visitor) const = 0;

  virtual nlohmann::json ToJson() const = 0;
  std::vector<std::unique_ptr<Expression>> DestroyAndGetChildren();

 private:
  catalog::SqlType type_;
  bool nullable_;

 protected:
  std::vector<std::unique_ptr<Expression>> children_;
};

class UnaryExpression : public Expression {
 public:
  UnaryExpression(catalog::SqlType type, bool nullable,
                  std::unique_ptr<Expression> child);
  virtual ~UnaryExpression() = default;

  Expression& Child();
  const Expression& Child() const;
};

class BinaryExpression : public Expression {
 public:
  BinaryExpression(catalog::SqlType type, bool nullable,
                   std::unique_ptr<Expression> left_child,
                   std::unique_ptr<Expression> right_child);
  virtual ~BinaryExpression() = default;

  Expression& LeftChild();
  const Expression& LeftChild() const;
  Expression& RightChild();
  const Expression& RightChild() const;
};

}  // namespace kush::plan