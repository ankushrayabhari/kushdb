#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"

namespace kush::plan {

class Operator {
 public:
  Operator* parent;
  Operator();
  virtual ~Operator() = default;
  virtual nlohmann::json ToJson() const = 0;
};

class Scan final : public Operator {
 public:
  const std::string relation;

  Scan(const std::string& rel);
  nlohmann::json ToJson() const override;
};

class Select final : public Operator {
 public:
  std::unique_ptr<Expression> expression;
  std::unique_ptr<Operator> child;
  Select(std::unique_ptr<Operator> c, std::unique_ptr<Expression> e);
  nlohmann::json ToJson() const override;
};

class Output final : public Operator {
 public:
  std::unique_ptr<Operator> child;
  Output(std::unique_ptr<Operator> c);
  nlohmann::json ToJson() const override;
};

class HashJoin final : public Operator {
 public:
  std::unique_ptr<Operator> left;
  std::unique_ptr<Operator> right;
  std::unique_ptr<ColumnRefExpression> left_column_;
  std::unique_ptr<ColumnRefExpression> right_column_;

  HashJoin(std::unique_ptr<Operator> l, std::unique_ptr<Operator> r,
           std::unique_ptr<ColumnRefExpression> left_column_,
           std::unique_ptr<ColumnRefExpression> right_column_);
  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan