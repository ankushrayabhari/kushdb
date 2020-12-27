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
  SelectOperator(OperatorSchema schema, std::unique_ptr<Operator> child,
                 std::unique_ptr<Expression> expression);

  const Expression& Expr() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::unique_ptr<Expression> expression_;
};

}  // namespace kush::plan
