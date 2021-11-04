#include "plan/operator/skinner_join_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

SkinnerJoinOperator::SkinnerJoinOperator(
    OperatorSchema schema, std::vector<std::unique_ptr<Operator>> children,
    std::vector<std::unique_ptr<Expression>> conditions)
    : Operator(std::move(schema), std::move(children)),
      conditions_(std::move(conditions)) {}

std::vector<std::reference_wrapper<const kush::plan::Expression>>
SkinnerJoinOperator::Conditions() const {
  return util::ImmutableReferenceVector(conditions_);
}

nlohmann::json SkinnerJoinOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SKINNER_JOIN";

  for (auto& c : Children()) {
    j["relations"].push_back(c.get().ToJson());
  }

  for (auto& cond : conditions_) {
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