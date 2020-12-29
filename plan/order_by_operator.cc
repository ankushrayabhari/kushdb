#include "plan/order_by_operator.h"

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

OrderByOperator::OrderByOperator(
    OperatorSchema schema, std::unique_ptr<Operator> child,
    std::vector<std::unique_ptr<Expression>> key_exprs)
    : UnaryOperator(std::move(schema), std::move(child)),
      key_exprs_(std::move(key_exprs)) {}

nlohmann::json OrderByOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "ORDER_BY";
  j["child"] = Child().ToJson();

  for (const auto& expr : key_exprs_) {
    j["group_by"].push_back(expr->ToJson());
  }

  j["output"] = Schema().ToJson();
  return j;
}

void OrderByOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}

void OrderByOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  return visitor.Visit(*this);
}

std::vector<std::reference_wrapper<const Expression>>
OrderByOperator::KeyExprs() const {
  return util::ImmutableReferenceVector(key_exprs_);
}

}  // namespace kush::plan