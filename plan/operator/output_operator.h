#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/operator/operator.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class OutputOperator final : public UnaryOperator {
 public:
  explicit OutputOperator(std::unique_ptr<Operator> child);

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan