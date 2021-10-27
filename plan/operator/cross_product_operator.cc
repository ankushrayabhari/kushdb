#include "plan/operator/cross_product_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

CrossProductOperator::CrossProductOperator(
    OperatorSchema schema, std::vector<std::unique_ptr<Operator>> children)
    : Operator(std::move(schema), std::move(children)) {}

nlohmann::json CrossProductOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "CROSS_PRODUCT";
  for (auto child : Children()) {
    j["children"].push_back(child.get().ToJson());
  }
  j["output"] = Schema().ToJson();
  return j;
}

void CrossProductOperator::Accept(OperatorVisitor& visitor) {
  visitor.Visit(*this);
}

void CrossProductOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

}  // namespace kush::plan