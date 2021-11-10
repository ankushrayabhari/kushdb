#include "plan/operator/skinner_join_operator.h"

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/union_find.h"
#include "util/vector_util.h"

namespace kush::plan {

std::vector<std::vector<std::unique_ptr<ColumnRefExpression>>>
SeparateEqualityConditions(
    std::vector<std::unique_ptr<Expression>>& conditions) {
  absl::flat_hash_set<int> equality_condition_idx;
  std::vector<std::unique_ptr<ColumnRefExpression>> column_refs;
  absl::flat_hash_map<std::pair<int, int>, int> column_ref_to_idx;
  absl::flat_hash_set<std::pair<int, int>> edges;
  for (int i = 0; i < conditions.size(); i++) {
    auto pred = conditions[i].get();
    if (auto eq = dynamic_cast<kush::plan::BinaryArithmeticExpression*>(pred)) {
      if (eq->OpType() == kush::plan::BinaryArithmeticExpressionType::EQ) {
        if (auto left_column = dynamic_cast<kush::plan::ColumnRefExpression*>(
                &eq->LeftChild())) {
          if (auto right_column =
                  dynamic_cast<kush::plan::ColumnRefExpression*>(
                      &eq->RightChild())) {
            equality_condition_idx.insert(i);

            std::pair<int, int> left_key = {left_column->GetChildIdx(),
                                            left_column->GetColumnIdx()};
            std::pair<int, int> right_key = {right_column->GetChildIdx(),
                                             right_column->GetColumnIdx()};

            auto children = eq->DestroyAndGetChildren();
            if (!column_ref_to_idx.contains(left_key)) {
              column_ref_to_idx[left_key] = column_refs.size();

              std::unique_ptr<ColumnRefExpression> ref(
                  dynamic_cast<ColumnRefExpression*>(children[0].release()));
              column_refs.emplace_back(std::move(ref));
            }

            if (!column_ref_to_idx.contains(right_key)) {
              column_ref_to_idx[right_key] = column_refs.size();

              std::unique_ptr<ColumnRefExpression> ref(
                  dynamic_cast<ColumnRefExpression*>(children[1].release()));
              column_refs.push_back(std::move(ref));
            }

            edges.insert(
                {column_ref_to_idx[left_key], column_ref_to_idx[right_key]});
          }
        }
      }
    }
  }

  // create a set for each column_ref
  std::vector<int> union_find(column_refs.size());
  for (int i = 0; i < union_find.size(); i++) {
    union_find[i] = i;
  }
  // merge all connected col_refs
  for (auto [l1, l2] : edges) {
    util::UnionFind::Union(union_find, l1, l2);
  }

  // output the equality conditions for each set
  std::vector<std::vector<std::unique_ptr<ColumnRefExpression>>>
      equality_conditions;
  absl::flat_hash_map<int, int> id_to_set_idx;
  for (int i = 0; i < column_refs.size(); i++) {
    auto id = util::UnionFind::Find(union_find, i);
    if (!id_to_set_idx.contains(id)) {
      id_to_set_idx[id] = equality_conditions.size();
      equality_conditions.emplace_back();
    }

    equality_conditions[id_to_set_idx.at(id)].push_back(
        std::move(column_refs[i]));
  }

  std::vector<std::unique_ptr<Expression>> new_conditions;
  for (int i = 0; i < conditions.size(); i++) {
    if (!equality_condition_idx.contains(i)) {
      new_conditions.push_back(std::move(conditions[i]));
    }
  }
  conditions = std::move(new_conditions);

  return equality_conditions;
}

SkinnerJoinOperator::SkinnerJoinOperator(
    OperatorSchema schema, std::vector<std::unique_ptr<Operator>> children,
    std::vector<std::unique_ptr<Expression>> conditions)
    : Operator(std::move(schema), std::move(children)),
      equality_conditions_(SeparateEqualityConditions(conditions)),
      general_conditions_(std::move(conditions)) {}

std::vector<std::reference_wrapper<const kush::plan::Expression>>
SkinnerJoinOperator::GeneralConditions() const {
  return util::ImmutableReferenceVector(general_conditions_);
}

std::vector<std::reference_wrapper<kush::plan::Expression>>
SkinnerJoinOperator::MutableGeneralConditions() {
  return util::ReferenceVector(general_conditions_);
}

std::vector<
    std::vector<std::reference_wrapper<const kush::plan::ColumnRefExpression>>>
SkinnerJoinOperator::EqualityConditions() const {
  std::vector<std::vector<
      std::reference_wrapper<const kush::plan::ColumnRefExpression>>>
      result;
  for (auto& v : equality_conditions_) {
    result.push_back(util::ImmutableReferenceVector(v));
  }
  return result;
}

std::vector<
    std::vector<std::reference_wrapper<kush::plan::ColumnRefExpression>>>
SkinnerJoinOperator::MutableEqualityConditions() {
  std::vector<
      std::vector<std::reference_wrapper<kush::plan::ColumnRefExpression>>>
      result;
  for (auto& v : equality_conditions_) {
    result.push_back(util::ReferenceVector(v));
  }
  return result;
}

nlohmann::json SkinnerJoinOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SKINNER_JOIN";

  for (auto& c : Children()) {
    j["relations"].push_back(c.get().ToJson());
  }

  for (auto& conds : equality_conditions_) {
    nlohmann::json eq;
    for (auto& cond : conds) {
      eq.push_back(cond->ToJson());
    }
    j["eq_cond"].push_back(eq);
  }

  for (auto& cond : general_conditions_) {
    j["cond"].push_back(cond->ToJson());
  }

  j["output"] = Schema().ToJson();
  return j;
}

void SkinnerJoinOperator::Accept(OperatorVisitor& visitor) {
  visitor.Visit(*this);
}

void SkinnerJoinOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

}  // namespace kush::plan