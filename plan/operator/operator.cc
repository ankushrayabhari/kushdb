#include "plan/operator/operator.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

Operator::Operator(OperatorSchema schema,
                   std::vector<std::unique_ptr<Operator>> children)
    : parent_(nullptr),
      schema_(std::move(schema)),
      children_(std::move(children)) {
  for (auto& child : children_) {
    child->parent_ = this;
  }
}

std::vector<std::reference_wrapper<Operator>> Operator::Children() {
  return util::ReferenceVector(children_);
}

std::vector<std::reference_wrapper<const Operator>> Operator::Children() const {
  return util::ImmutableReferenceVector(children_);
}

const OperatorSchema& Operator::Schema() const { return schema_; }

OperatorSchema& Operator::MutableSchema() { return schema_; }

std::optional<std::reference_wrapper<Operator>> Operator::Parent() {
  if (parent_ == nullptr) {
    return std::nullopt;
  }

  return *parent_;
}

UnaryOperator::UnaryOperator(OperatorSchema schema,
                             std::unique_ptr<Operator> child)
    : Operator(std::move(schema), util::MakeVector(std::move(child))) {}

Operator& UnaryOperator::Child() { return *children_[0]; }

const Operator& UnaryOperator::Child() const { return *children_[0]; }

BinaryOperator::BinaryOperator(OperatorSchema schema,
                               std::unique_ptr<Operator> left_child,
                               std::unique_ptr<Operator> right_child)
    : Operator(std::move(schema), util::MakeVector(std::move(left_child),
                                                   std::move(right_child))) {}

Operator& BinaryOperator::LeftChild() { return *children_[0]; }

const Operator& BinaryOperator::LeftChild() const { return *children_[0]; }

Operator& BinaryOperator::RightChild() { return *children_[1]; }

const Operator& BinaryOperator::RightChild() const { return *children_[1]; }

}  // namespace kush::plan