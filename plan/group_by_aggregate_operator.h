#pragma once

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class GroupByAggregateOperator final : public UnaryOperator {
 public:
  GroupByAggregateOperator(
      OperatorSchema schema, std::unique_ptr<Operator> child,
      std::vector<std::unique_ptr<Expression>> group_by_exprs,
      std::vector<std::unique_ptr<AggregateExpression>> aggregate_exprs);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
  std::vector<std::reference_wrapper<Expression>> GroupByExprs();
  std::vector<std::reference_wrapper<AggregateExpression>> AggExprs();

 private:
  std::vector<std::unique_ptr<Expression>> group_by_exprs_;
  std::vector<std::unique_ptr<AggregateExpression>> aggregate_exprs_;
};

}  // namespace kush::plan