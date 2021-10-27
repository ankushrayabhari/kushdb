#pragma once

#include <memory>
#include <vector>

#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

class OrderByOperator final : public UnaryOperator {
 public:
  OrderByOperator(OperatorSchema schema, std::unique_ptr<Operator> child,
                  std::vector<std::unique_ptr<ColumnRefExpression>> key_exprs,
                  std::vector<bool> asc);

  std::vector<std::reference_wrapper<const ColumnRefExpression>> KeyExprs()
      const;
  const std::vector<bool>& Ascending() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::vector<std::unique_ptr<ColumnRefExpression>> key_exprs_;
  std::vector<bool> asc_;
};

}  // namespace kush::plan