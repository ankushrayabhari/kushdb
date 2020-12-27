#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class OutputOperator final : public UnaryOperator {
 public:
  OutputOperator(std::unique_ptr<Operator> child);

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;
};

}  // namespace kush::plan
