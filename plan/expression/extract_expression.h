#pragma once

#include <optional>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

enum class ExtractValue { YEAR };

class ExtractExpression : public UnaryExpression {
 public:
  ExtractExpression(std::unique_ptr<Expression> child, ExtractValue value);

  catalog::SqlType Type() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

  ExtractValue ValueToExtract() const;

 private:
  ExtractValue value_to_extract_;
};

}  // namespace kush::plan