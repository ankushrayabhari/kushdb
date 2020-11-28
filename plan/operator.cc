#include "plan/operator.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

Operator::Operator(OperatorSchema schema,
                   std::vector<std::unique_ptr<Operator>> children)
    : parent_(nullptr),
      schema_(std::move(schema)),
      children_(std::move(children)) {
  for (auto& child : children_) {
    child->SetParent(this);
  }
}

const OperatorSchema& Operator::Schema() const { return schema_; }

std::optional<std::reference_wrapper<Operator>> Operator::Parent() {
  if (parent_ == nullptr) {
    return std::nullopt;
  }

  return *parent_;
}

void Operator::SetParent(Operator* parent) { parent_ = parent; }

UnaryOperator::UnaryOperator(OperatorSchema schema,
                             std::unique_ptr<Operator> child)
    : Operator(std::move(schema), make_vector(std::move(child))) {}

Operator& UnaryOperator::Child() { return *children_[0]; }

const Operator& UnaryOperator::Child() const { return *children_[0]; }

BinaryOperator::BinaryOperator(OperatorSchema schema,
                               std::unique_ptr<Operator> left_child,
                               std::unique_ptr<Operator> right_child)
    : Operator(std::move(schema),
               make_vector(std::move(left_child), std::move(right_child))) {}

Operator& BinaryOperator::LeftChild() { return *children_[0]; }

const Operator& BinaryOperator::LeftChild() const { return *children_[0]; }

Operator& BinaryOperator::RightChild() { return *children_[1]; }

const Operator& BinaryOperator::RightChild() const { return *children_[1]; }

Scan::Scan(OperatorSchema schema, const std::string& rel)
    : Operator(std::move(schema), {}), relation(rel) {}

nlohmann::json Scan::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation;
  j["output"] = Schema().ToJson();
  return j;
}

void Scan::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

Select::Select(OperatorSchema schema, std::unique_ptr<Operator> child,
               std::unique_ptr<Expression> e)
    : UnaryOperator(std::move(schema), {std::move(child)}),
      expression(std::move(e)) {
  children_[0]->SetParent(this);
}

nlohmann::json Select::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = children_[0]->ToJson();
  j["expression"] = expression->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void Select::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

Output::Output(std::unique_ptr<Operator> child)
    : UnaryOperator(OperatorSchema(), {std::move(child)}) {}

nlohmann::json Output::ToJson() const {
  nlohmann::json j;
  j["op"] = "OUTPUT";
  j["relation"] = Child().ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void Output::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

HashJoin::HashJoin(OperatorSchema schema, std::unique_ptr<Operator> left,
                   std::unique_ptr<Operator> right,
                   std::unique_ptr<ColumnRefExpression> left_column,
                   std::unique_ptr<ColumnRefExpression> right_column)
    : BinaryOperator(std::move(schema), std::move(left), std::move(right)),
      left_column_(std::move(left_column)),
      right_column_(std::move(right_column)) {}

nlohmann::json HashJoin::ToJson() const {
  nlohmann::json j;
  j["op"] = "HASH_JOIN";
  j["left_relation"] = LeftChild().ToJson();
  j["right_relation"] = RightChild().ToJson();

  nlohmann::json eq;
  eq["left"] = left_column_->ToJson();
  eq["right"] = right_column_->ToJson();
  j["keys"].push_back(eq);

  j["output"] = Schema().ToJson();
  return j;
}

void HashJoin::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

}  // namespace kush::plan