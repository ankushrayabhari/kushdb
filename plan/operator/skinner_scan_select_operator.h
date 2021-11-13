#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class SkinnerScanSelectOperator final : public Operator {
 public:
  SkinnerScanSelectOperator(OperatorSchema select_schema,
                            OperatorSchema scan_schema,
                            const catalog::Table& relation,
                            std::vector<std::unique_ptr<Expression>> filters);

  const catalog::Table& Relation() const;

  const OperatorSchema& ScanSchema() const;
  OperatorSchema& MutableScanSchema();

  std::vector<std::reference_wrapper<const Expression>> Filters() const;
  std::vector<std::reference_wrapper<Expression>> MutableFilters();

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  OperatorSchema scan_schema_;
  const catalog::Table& relation_;
  std::vector<std::unique_ptr<Expression>> filters_;
};

}  // namespace kush::plan
