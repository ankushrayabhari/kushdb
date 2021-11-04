#pragma once

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class GroupByAggregateOperator final : public UnaryOperator {
 public:
  GroupByAggregateOperator(
      OperatorSchema schema, std::unique_ptr<Operator> child,
      std::vector<std::unique_ptr<Expression>> group_by_exprs,
      std::vector<std::unique_ptr<AggregateExpression>> aggregate_exprs);

  std::vector<std::reference_wrapper<const Expression>> GroupByExprs() const;
  std::vector<std::reference_wrapper<const AggregateExpression>> AggExprs()
      const;
  std::vector<std::reference_wrapper<AggregateExpression>> MutableAggExprs();

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::unique_ptr<Expression>> group_by_exprs_;
  std::vector<std::unique_ptr<AggregateExpression>> aggregate_exprs_;
};

}  // namespace kush::plan