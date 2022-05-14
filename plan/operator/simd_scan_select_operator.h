#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class SimdScanSelectOperator final : public Operator {
 public:
  SimdScanSelectOperator(
      OperatorSchema select_schema, OperatorSchema scan_schema,
      const catalog::Table& relation,
      std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>>
          filters);

  const catalog::Table& Relation() const;

  const OperatorSchema& ScanSchema() const;
  OperatorSchema& MutableScanSchema();

  const std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>>&
  Filters() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  OperatorSchema scan_schema_;
  const catalog::Table& relation_;
  std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>>
      filters_;
};

}  // namespace kush::plan
