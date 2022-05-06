#include "plan/operator/scan_select_operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

ScanSelectOperator::ScanSelectOperator(
    OperatorSchema select_schema, OperatorSchema scan_schema,
    const catalog::Table& relation,
    std::vector<std::unique_ptr<Expression>> filters)
    : Operator(std::move(select_schema), {}),
      scan_schema_(std::move(scan_schema)),
      relation_(relation),
      filters_(std::move(filters)) {}

std::vector<std::reference_wrapper<const Expression>>
ScanSelectOperator::Filters() const {
  return util::ImmutableReferenceVector(filters_);
}

std::vector<std::reference_wrapper<Expression>>
ScanSelectOperator::MutableFilters() {
  return util::ReferenceVector(filters_);
}

const catalog::Table& ScanSelectOperator::Relation() const { return relation_; }

const OperatorSchema& ScanSelectOperator::ScanSchema() const {
  return scan_schema_;
}

OperatorSchema& ScanSelectOperator::MutableScanSchema() { return scan_schema_; }

void ScanSelectOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}
void ScanSelectOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  return visitor.Visit(*this);
}

nlohmann::json ScanSelectOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SKINER_SCAN_SELECT";
  j["relation"] = relation_.Name();
  j["scan_output"] = scan_schema_.ToJson();
  j["output"] = Schema().ToJson();
  for (const auto& f : filters_) {
    j["filters"].push_back(f->ToJson());
  }

  return j;
}

}  // namespace kush::plan
