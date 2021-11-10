#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class SkinnerJoinOperator final : public Operator {
 public:
  SkinnerJoinOperator(OperatorSchema schema,
                      std::vector<std::unique_ptr<Operator>> children,
                      std::vector<std::unique_ptr<Expression>> conditions);

  std::vector<std::reference_wrapper<const plan::Expression>>
  GeneralConditions() const;
  std::vector<std::reference_wrapper<plan::Expression>>
  MutableGeneralConditions();

  std::vector<std::vector<
      std::reference_wrapper<const kush::plan::ColumnRefExpression>>>
  EqualityConditions() const;
  std::vector<
      std::vector<std::reference_wrapper<kush::plan::ColumnRefExpression>>>
  MutableEqualityConditions();

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::vector<std::unique_ptr<ColumnRefExpression>>>
      equality_conditions_;
  std::vector<std::unique_ptr<Expression>> general_conditions_;
};

}  // namespace kush::plan