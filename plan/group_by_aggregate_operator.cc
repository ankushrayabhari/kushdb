#include "plan/group_by_aggregate_operator.h"

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

GroupByAggregateOperator::GroupByAggregateOperator(
    OperatorSchema schema, std::unique_ptr<Operator> child,
    std::vector<std::unique_ptr<Expression>> group_by_exprs)
    : UnaryOperator(std::move(schema), std::move(child)),
      group_by_exprs_(std::move(group_by_exprs)) {}

nlohmann::json GroupByAggregateOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "GROUP_BY_AGG";
  j["child"] = Child().ToJson();

  for (const auto& expr : group_by_exprs_) {
    j["group_by"].push_back(expr->ToJson());
  }

  j["output"] = Schema().ToJson();
  return j;
}

void GroupByAggregateOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}

std::vector<std::reference_wrapper<Expression>>
GroupByAggregateOperator::GroupByExprs() {
  return util::ReferenceVector(group_by_exprs_);
}

}  // namespace kush::plan