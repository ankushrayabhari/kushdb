#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class CrossProductOperator final : public Operator {
 public:
  CrossProductOperator(OperatorSchema schema,
                       std::vector<std::unique_ptr<Operator>> children);

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan