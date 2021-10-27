#include "plan/operator/output_operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

OutputOperator::OutputOperator(std::unique_ptr<Operator> child)
    : UnaryOperator(OperatorSchema(), {std::move(child)}) {}

nlohmann::json OutputOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "OUTPUT";
  j["relation"] = Child().ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void OutputOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

void OutputOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

}  // namespace kush::plan
