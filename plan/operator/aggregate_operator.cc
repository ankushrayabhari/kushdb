#include "plan/operator/aggregate_operator.h"

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

AggregateOperator::AggregateOperator(
    OperatorSchema schema, std::unique_ptr<Operator> child,
    std::vector<std::unique_ptr<AggregateExpression>> aggregate_exprs)
    : UnaryOperator(std::move(schema), std::move(child)),
      aggregate_exprs_(std::move(aggregate_exprs)) {}

nlohmann::json AggregateOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "AGG";
  j["child"] = Child().ToJson();

  for (const auto& expr : aggregate_exprs_) {
    j["agg"].push_back(expr->ToJson());
  }

  j["output"] = Schema().ToJson();
  return j;
}

void AggregateOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}

void AggregateOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  return visitor.Visit(*this);
}

std::vector<std::reference_wrapper<const AggregateExpression>>
AggregateOperator::AggExprs() const {
  return util::ImmutableReferenceVector(aggregate_exprs_);
}

std::vector<std::reference_wrapper<AggregateExpression>>
AggregateOperator::MutableAggExprs() {
  return util::ReferenceVector(aggregate_exprs_);
}

}  // namespace kush::plan