#include "plan/select_operator.h"

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

SelectOperator::SelectOperator(OperatorSchema schema,
                               std::unique_ptr<Operator> child,
                               std::unique_ptr<Expression> e)
    : UnaryOperator(std::move(schema), {std::move(child)}),
      expression(std::move(e)) {
  children_[0]->SetParent(this);
}

nlohmann::json SelectOperator::ToJson() const {
  nlohmann::json j;
  j["op"] = "SELECT";
  j["relation"] = children_[0]->ToJson();
  j["expression"] = expression->ToJson();
  j["output"] = Schema().ToJson();
  return j;
}

void SelectOperator::Accept(OperatorVisitor& visitor) { visitor.Visit(*this); }

}  // namespace kush::plan
