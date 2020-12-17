#include "plan/scan_operator.h"

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

ScanOperator::ScanOperator(OperatorSchema schema, std::string_view relation)
    : Operator(std::move(schema), {}), relation_(relation) {}

nlohmann::json ScanOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation_;
  j["output"] = Schema().ToJson();
  return j;
}

void ScanOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

std::string_view ScanOperator::Relation() const { return relation_; }

}  // namespace kush::plan