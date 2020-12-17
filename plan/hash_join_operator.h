#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class HashJoinOperator final : public BinaryOperator {
 public:
  HashJoinOperator(OperatorSchema schema, std::unique_ptr<Operator> left,
                   std::unique_ptr<Operator> right,
                   std::unique_ptr<ColumnRefExpression> left_column_,
                   std::unique_ptr<ColumnRefExpression> right_column_);
  nlohmann::json ToJson() const override;
  void Accept(OperatorVisitor& visitor) override;
  ColumnRefExpression& LeftColumn();
  ColumnRefExpression& RightColumn();

 private:
  std::unique_ptr<ColumnRefExpression> left_column_;
  std::unique_ptr<ColumnRefExpression> right_column_;
};

}  // namespace kush::plan