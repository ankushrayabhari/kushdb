#include "plan/scan_operator.h"

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

ScanOperator::ScanOperator(OperatorSchema schema,
                           const catalog::Table& relation)
    : Operator(std::move(schema), {}), relation_(relation) {}

nlohmann::json ScanOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation_.Name();
  j["output"] = Schema().ToJson();
  return j;
}

void ScanOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

void ScanOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

const catalog::Table& ScanOperator::Relation() const { return relation_; }

}  // namespace kush::plan