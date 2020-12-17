#pragma once

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class ScanOperator final : public Operator {
 public:
  ScanOperator(OperatorSchema schema, std::string_view relation);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
  std::string_view Relation() const;

 private:
  std::string relation_;
};

}  // namespace kush::plan