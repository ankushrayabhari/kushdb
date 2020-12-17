#include "plan/hash_join_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

HashJoinOperator::HashJoinOperator(
    OperatorSchema schema, std::unique_ptr<Operator> left,
    std::unique_ptr<Operator> right,
    std::unique_ptr<ColumnRefExpression> left_column,
    std::unique_ptr<ColumnRefExpression> right_column)
    : BinaryOperator(std::move(schema), std::move(left), std::move(right)),
      left_column_(std::move(left_column)),
      right_column_(std::move(right_column)) {}

ColumnRefExpression& HashJoinOperator::LeftColumn() { return *left_column_; }

ColumnRefExpression& HashJoinOperator::RightColumn() { return *right_column_; }

nlohmann::json HashJoinOperator::ToJson() const {
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

void HashJoinOperator::Accept(OperatorVisitor& visitor) {
  visitor.Visit(*this);
}

}  // namespace kush::plan