#include "plan/scan_operator.h"

#include <string>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

ScanOperator::ScanOperator(OperatorSchema schema, const std::string& rel)
    : Operator(std::move(schema), {}), relation(rel) {}

nlohmann::json ScanOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SCAN";
  j["relation"] = relation;
  j["output"] = Schema().ToJson();
  return j;
}

void ScanOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

}  // namespace kush::plan