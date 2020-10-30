#include "plan/operator.h"

#include <string>

#include "nlohmann/json.hpp"

namespace kush::plan {

Operator::Operator() : parent(nullptr) {}

Scan::Scan(const std::string& rel) : relation(rel) {}

nlohmann::json Scan::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation;
  return j;
}

Select::Select(std::unique_ptr<Operator> c, std::unique_ptr<Expression> e)
    : expression(std::move(e)), child(std::move(c)) {
  child->parent = this;
}

nlohmann::json Select::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = child->ToJson();
  j["expression"] = expression->ToJson();
  return j;
}

Output::Output(std::unique_ptr<Operator> c) : child(std::move(c)) {
  child->parent = this;
}

nlohmann::json Output::ToJson() const {
  nlohmann::json j;
  j["op"] = "OUTPUT";
  j["relation"] = child->ToJson();
  return j;
}

HashJoin::HashJoin(std::unique_ptr<Operator> l, std::unique_ptr<Operator> r,
                   std::unique_ptr<ColumnRefExpression> left_column,
                   std::unique_ptr<ColumnRefExpression> right_column)
    : left(std::move(l)),
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
  return j;
}

}  // namespace kush::plan