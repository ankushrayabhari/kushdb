#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class SkinnerJoinOperator final : public Operator {
 public:
  SkinnerJoinOperator(
      OperatorSchema schema, std::vector<std::unique_ptr<Operator>> children,
      std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions);

  std::vector<std::reference_wrapper<const plan::BinaryArithmeticExpression>>
  Conditions() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions_;
};

}  // namespace kush::plan