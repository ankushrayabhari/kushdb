#include "plan/operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

Operator::Operator(OperatorSchema schema)
    : parent(nullptr), schema_(std::move(schema)) {}

const OperatorSchema& Operator::Schema() const { return schema_; }

Scan::Scan(OperatorSchema schema, const std::string& rel)
    : Operator(std::move(schema)), relation(rel) {}

nlohmann::json Scan::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation;
  j["output"] = Schema().ToJson();
  return j;
}

void Scan::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

Select::Select(OperatorSchema schema, std::unique_ptr<Operator> c,
               std::unique_ptr<Expression> e)
    : Operator(std::move(schema)),
      expression(std::move(e)),
      child(std::move(c)) {
  child->parent = this;
}

nlohmann::json Select::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = child->ToJson();
  j["expression"] = expression->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void Select::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

Output::Output(OperatorSchema schema, std::unique_ptr<Operator> c)
    : Operator(std::move(schema)), child(std::move(c)) {
  child->parent = this;
}

nlohmann::json Output::ToJson() const {
  nlohmann::json j;
  j["op"] = "OUTPUT";
  j["relation"] = child->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void Output::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

HashJoin::HashJoin(OperatorSchema schema, std::unique_ptr<Operator> l,
                   std::unique_ptr<Operator> r,
                   std::unique_ptr<ColumnRefExpression> left_column,
                   std::unique_ptr<ColumnRefExpression> right_column)
    : Operator(std::move(schema)),
      left(std::move(l)),
      right(std::move(r)),
      left_column_(std::move(left_column)),
      right_column_(std::move(right_column)) {
  left->parent = this;
  right->parent = this;
}

nlohmann::json HashJoin::ToJson() const {
  nlohmann::json j;
  j["op"] = "HASH_JOIN";
  j["left_relation"] = left->ToJson();
  j["right_relation"] = right->ToJson();

  nlohmann::json eq;
  eq["left"] = left_column_->ToJson();
  eq["right"] = right_column_->ToJson();
  j["keys"].push_back(eq);

  j["output"] = Schema().ToJson();
  return j;
}

void HashJoin::Accept(OperatorVisitor& visitor) { return visitor.Visit(*this); }

}  // namespace kush::plan