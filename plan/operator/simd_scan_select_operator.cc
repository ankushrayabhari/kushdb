#include "plan/operator/simd_scan_select_operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

SimdScanSelectOperator::SimdScanSelectOperator(
    OperatorSchema select_schema, OperatorSchema scan_schema,
    const catalog::Table& relation,
    std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>>
        filters)
    : Operator(std::move(select_schema), {}),
      scan_schema_(std::move(scan_schema)),
      relation_(relation),
      filters_(std::move(filters)) {}

const std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>>&
SimdScanSelectOperator::Filters() const {
  return filters_;
}

const catalog::Table& SimdScanSelectOperator::Relation() const {
  return relation_;
}

const OperatorSchema& SimdScanSelectOperator::ScanSchema() const {
  return scan_schema_;
}

OperatorSchema& SimdScanSelectOperator::MutableScanSchema() {
  return scan_schema_;
}

void SimdScanSelectOperator::Accept(OperatorVisitor& visitor) {
  return visitor.Visit(*this);
}
void SimdScanSelectOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  return visitor.Visit(*this);
}

nlohmann::json SimdScanSelectOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SIMD_SCAN_SELECT";
  j["relation"] = relation_.Name();
  j["scan_output"] = scan_schema_.ToJson();
  j["output"] = Schema().ToJson();
  for (int i = 0; i < filters_.size(); i++) {
    for (const auto& f : filters_[i]) {
      j[i].push_back(f->ToJson());
    }
  }

  return j;
}

}  // namespace kush::plan
