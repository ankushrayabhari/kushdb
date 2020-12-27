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

  std::string_view Relation() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::string relation_;
};

}  // namespace kush::plan