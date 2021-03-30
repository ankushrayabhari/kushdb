#include "plan/skinner_join_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

SkinnerJoinOperator::SkinnerJoinOperator(
    OperatorSchema schema, std::vector<std::unique_ptr<Operator>> children,
    std::vector<std::vector<std::unique_ptr<ColumnRefExpression>>> key_columns)
    : Operator(std::move(schema), std::move(children)) {}

std::vector<std::vector<std::reference_wrapper<const ColumnRefExpression>>>
SkinnerJoinOperator::KeyColumns() const {
  std::vector<std::vector<std::reference_wrapper<const ColumnRefExpression>>>
      output;
  output.reserve(key_columns_.size());
  for (auto& v : key_columns_) {
    output.push_back(util::ImmutableReferenceVector(v));
  }
  return output;
}

nlohmann::json SkinnerJoinOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SKINNER_JOIN";

  for (auto& c : Children()) {
    j["relations"].push_back(c.get().ToJson());
  }

  for (auto& key_cols : key_columns_) {
    nlohmann::json eq;
    for (auto& c : key_cols) {
      eq.push_back(c->ToJson());
    }
    j["keys"].push_back(eq);
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