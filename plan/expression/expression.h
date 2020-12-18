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
  Expression(catalog::SqlType type,
             std::vector<std::unique_ptr<Expression>> children);
  virtual ~Expression() = default;
  virtual nlohmann::json ToJson() const = 0;
  virtual void Accept(ExpressionVisitor& visitor) = 0;
  std::vector<std::reference_wrapper<Expression>> Children();
  catalog::SqlType Type() const;

 private:
  catalog::SqlType type_;

 protected:
  std::vector<std::unique_ptr<Expression>> children_;
};

class UnaryExpression : public Expression {
 public:
  UnaryExpression(catalog::SqlType type, std::unique_ptr<Expression> child);
  virtual ~UnaryExpression() = default;
  Expression& Child();
  const Expression& Child() const;
};

class BinaryExpression : public Expression {
 public:
  BinaryExpression(catalog::SqlType type,
                   std::unique_ptr<Expression> left_child,
                   std::unique_ptr<Expression> right_child);
  virtual ~BinaryExpression() = default;
  Expression& LeftChild();
  const Expression& LeftChild() const;
  Expression& RightChild();
  const Expression& RightChild() const;
};

}  // namespace kush::plan