#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class Operator : public OperatorSchemaProvider {
 public:
  Operator(OperatorSchema schema,
           std::vector<std::unique_ptr<Operator>> children);
  virtual ~Operator() = default;

  const OperatorSchema& Schema() const override;

  virtual void Accept(OperatorVisitor& visitor) = 0;
  virtual void Accept(ImmutableOperatorVisitor& visitor) const = 0;

  std::optional<std::reference_wrapper<Operator>> Parent();
  std::optional<std::reference_wrapper<const Operator>> Parent() const;
  std::vector<std::reference_wrapper<Operator>> Children();
  std::vector<std::reference_wrapper<const Operator>> Children() const;

  virtual nlohmann::json ToJson() const = 0;

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

}  // namespace kush::plan