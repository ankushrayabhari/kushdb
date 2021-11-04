#include "plan/operator/select_operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/operator_visitor.h"

namespace kush::plan {

SelectOperator::SelectOperator(OperatorSchema schema,
                               std::unique_ptr<Operator> child,
                               std::unique_ptr<Expression> expression)
    : UnaryOperator(std::move(schema), {std::move(child)}),
      expression_(std::move(expression)) {}

nlohmann::json SelectOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = Child().ToJson();
  j["expression"] = expression_->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void SelectOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

void SelectOperator::Accept(ImmutableOperatorVisitor& visitor) const {
  visitor.Visit(*this);
}

const Expression& SelectOperator::Expr() const { return *expression_; }

Expression& SelectOperator::MutableExpr() { return *expression_; }

}  // namespace kush::plan
