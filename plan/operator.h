#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class Operator {
 public:
  Operator(OperatorSchema schema,
           std::vector<std::unique_ptr<Operator>> children);
  virtual ~Operator() = default;

  virtual nlohmann::json ToJson() const = 0;
  virtual void Accept(OperatorVisitor& visitor) = 0;
  const OperatorSchema& Schema() const;
  std::optional<std::reference_wrapper<Operator>> Parent();
  void SetParent(Operator* parent);
  std::vector<std::reference_wrapper<Operator>> Children();

 private:
  Operator* parent_;
  OperatorSchema schema_;

 protected:
  std::vector<std::unique_ptr<Operator>> children_;
};

class UnaryOperator : public Operator {
 public:
  UnaryOperator(OperatorSchema schema, std::unique_ptr<Operator> child);
  virtual ~UnaryOperator() = default;
  Operator& Child();
  const Operator& Child() const;
};

class BinaryOperator : public Operator {
 public:
  BinaryOperator(OperatorSchema schema, std::unique_ptr<Operator> left_child,
                 std::unique_ptr<Operator> right_child);
  virtual ~BinaryOperator() = default;
  Operator& LeftChild();
  const Operator& LeftChild() const;
  Operator& RightChild();
  const Operator& RightChild() const;
};

class Scan final : public Operator {
 public:
  const std::string relation;

  Scan(OperatorSchema schema, const std::string& rel);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class Select final : public UnaryOperator {
 public:
  std::unique_ptr<Expression> expression;
  Select(OperatorSchema schema, std::unique_ptr<Operator> child,
         std::unique_ptr<Expression> e);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class Output final : public UnaryOperator {
 public:
  Output(std::unique_ptr<Operator> child);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class HashJoin final : public BinaryOperator {
 public:
  HashJoin(OperatorSchema schema, std::unique_ptr<Operator> left,
           std::unique_ptr<Operator> right,
           std::unique_ptr<ColumnRefExpression> left_column_,
           std::unique_ptr<ColumnRefExpression> right_column_);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
  ColumnRefExpression& LeftColumn();
  ColumnRefExpression& RightColumn();

 private:
  std::unique_ptr<ColumnRefExpression> left_column_;
  std::unique_ptr<ColumnRefExpression> right_column_;
};

}  // namespace kush::plan