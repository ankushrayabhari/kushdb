#pragma once

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class OrderByOperator final : public UnaryOperator {
 public:
  OrderByOperator(OperatorSchema schema, std::unique_ptr<Operator> child,
                  std::vector<std::unique_ptr<Expression>> key_exprs);

  std::vector<std::reference_wrapper<const Expression>> KeyExprs() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::unique_ptr<Expression>> key_exprs_;
};

}  // namespace kush::plan