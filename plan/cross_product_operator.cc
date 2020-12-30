#include "plan/cross_product_operator.h"

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

CrossProductOperator::CrossProductOperator(OperatorSchema schema,
                                           std::unique_ptr<Operator> left,
                                           std::unique_ptr<Operator> right)
    : BinaryOperator(std::move(schema), std::move(left), std::move(right)) {}

nlohmann::json CrossProductOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "CROSS_PRODUCT";
  j["left_relation"] = LeftChild().ToJson();
  j["right_relation"] = RightChild().ToJson();
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