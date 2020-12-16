#pragma once

#include <string>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class ScanOperator final : public Operator {
 public:
  const std::string relation;

  ScanOperator(OperatorSchema schema, const std::string& rel);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

}  // namespace kush::plan