#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class CrossProductOperator final : public BinaryOperator {
 public:
  CrossProductOperator(OperatorSchema schema, std::unique_ptr<Operator> left,
                       std::unique_ptr<Operator> right);

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan