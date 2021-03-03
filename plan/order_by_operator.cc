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
    std::vector<std::unique_ptr<ColumnRefExpression>> key_exprs,
    std::vector<bool> asc)
    : UnaryOperator(std::move(schema), std::move(child)),
      key_exprs_(std::move(key_exprs)),
      asc_(std::move(asc)) {
  if (key_exprs_.size() != asc_.size()) {
    throw std::runtime_error(
        "Must have exactly as many asc flags as key exprs.");
  }
}

nlohmann::json OrderByOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "ORDER_BY";
  j["child"] = Child().ToJson();

  for (int i = 0; i < key_exprs_.size(); i++) {
    nlohmann::json key;
    key["expr"] = key_exprs_[i]->ToJson();
    key["asc"] = asc_[i];
    j["order_by"].push_back(key);
  }

  j["output"] = Schema().ToJson();
  return j;
}

const std::vector<bool>& OrderByOperator::Ascending() const { return asc_; }

void OrderByOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}

void OrderByOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  return visitor.Visit(*this);
}

std::vector<std::reference_wrapper<const ColumnRefExpression>>
OrderByOperator::KeyExprs() const {
  return util::ImmutableReferenceVector(key_exprs_);
}

}  // namespace kush::plan