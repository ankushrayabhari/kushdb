#include "plan/operator/hash_join_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

HashJoinOperator::HashJoinOperator(
    OperatorSchema schema, std::unique_ptr<Operator> left,
    std::unique_ptr<Operator> right,
    std::vector<std::unique_ptr<ColumnRefExpression>> left_columns,
    std::vector<std::unique_ptr<ColumnRefExpression>> right_columns)
    : BinaryOperator(std::move(schema), std::move(left), std::move(right)),
      left_columns_(std::move(left_columns)),
      right_columns_(std::move(right_columns)) {}

std::vector<std::reference_wrapper<const ColumnRefExpression>>
HashJoinOperator::LeftColumns() const {
  return util::ImmutableReferenceVector(left_columns_);
}

std::vector<std::reference_wrapper<const ColumnRefExpression>>
HashJoinOperator::RightColumns() const {
  return util::ImmutableReferenceVector(right_columns_);
}

nlohmann::json HashJoinOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "HASH_JOIN";
  j["left_relation"] = LeftChild().ToJson();
  j["right_relation"] = RightChild().ToJson();

  for (int i = 0; i < left_columns_.size(); i++) {
    nlohmann::json eq;
    eq["left"] = left_columns_[i]->ToJson();
    eq["right"] = right_columns_[i]->ToJson();
    j["keys"].push_back(eq);
  }

  j["output"] = Schema().ToJson();
  return j;
}

void HashJoinOperator::Accept(OperatorVisitor& visitor) {
  visitor.Visit(*this);
}

void HashJoinOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

}  // namespace kush::plan