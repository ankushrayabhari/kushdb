#pragma once
#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class Operator {
 public:
  Operator(OperatorSchema schema);
  virtual ~Operator() = default;
  virtual nlohmann::json ToJson() const = 0;
  virtual void Accept(OperatorVisitor& visitor) = 0;
  const OperatorSchema& Schema() const;

  Operator* parent;

 private:
  OperatorSchema schema_;
};

class Scan final : public Operator {
 public:
  const std::string relation;

  Scan(OperatorSchema schema, const std::string& rel);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class Select final : public Operator {
 public:
  std::unique_ptr<Expression> expression;
  std::unique_ptr<Operator> child;
  Select(OperatorSchema schema, std::unique_ptr<Operator> c,
         std::unique_ptr<Expression> e);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class Output final : public Operator {
 public:
  std::unique_ptr<Operator> child;
  Output(OperatorSchema schema, std::unique_ptr<Operator> c);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

class HashJoin final : public Operator {
 public:
  std::unique_ptr<Operator> left;
  std::unique_ptr<Operator> right;
  std::unique_ptr<ColumnRefExpression> left_column_;
  std::unique_ptr<ColumnRefExpression> right_column_;

  HashJoin(OperatorSchema schema, std::unique_ptr<Operator> l,
           std::unique_ptr<Operator> r,
           std::unique_ptr<ColumnRefExpression> left_column_,
           std::unique_ptr<ColumnRefExpression> right_column_);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

}  // namespace kush::plan