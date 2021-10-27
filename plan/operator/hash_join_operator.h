#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class HashJoinOperator final : public BinaryOperator {
 public:
  HashJoinOperator(
      OperatorSchema schema, std::unique_ptr<Operator> left,
      std::unique_ptr<Operator> right,
      std::vector<std::unique_ptr<ColumnRefExpression>> left_columns_,
      std::vector<std::unique_ptr<ColumnRefExpression>> right_columns_);

  std::vector<std::reference_wrapper<const ColumnRefExpression>> LeftColumns()
      const;
  std::vector<std::reference_wrapper<const ColumnRefExpression>> RightColumns()
      const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::unique_ptr<ColumnRefExpression>> left_columns_;
  std::vector<std::unique_ptr<ColumnRefExpression>> right_columns_;
};

}  // namespace kush::plan