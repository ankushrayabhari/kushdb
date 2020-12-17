#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class SelectOperator final : public UnaryOperator {
 public:
  std::unique_ptr<Expression> expression;
  SelectOperator(OperatorSchema schema, std::unique_ptr<Operator> child,
                 std::unique_ptr<Expression> e);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
};

}  // namespace kush::plan
