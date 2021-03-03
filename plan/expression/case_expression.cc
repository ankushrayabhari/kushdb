
#include "plan/expression/case_expression.h"

#include <memory>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

catalog::SqlType CalculateType(catalog::SqlType cond, catalog::SqlType left,
                               catalog::SqlType right) {
  if (cond != catalog::SqlType::BOOLEAN) {
    throw std::runtime_error("Non-boolean condition");
  }
  if (left != right) {
    throw std::runtime_error("Case must have same type arguments");
  }
  return left;
}

CaseExpression::CaseExpression(std::unique_ptr<Expression> cond,
                               std::unique_ptr<Expression> left,
                               std::unique_ptr<Expression> right)
    : Expression(CalculateType(cond->Type(), left->Type(), right->Type()),
                 util::MakeVector(std::move(cond), std::move(left),
                                  std::move(right))) {}

nlohmann::json CaseExpression::ToJson() const {
  nlohmann::json j;
  j["cond"] = Cond().ToJson();
  j["then"] = Then().ToJson();
  j["else"] = Else().ToJson();
  return j;
}

void CaseExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void CaseExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

const Expression& CaseExpression::Cond() const { return *children_[0]; }

const Expression& CaseExpression::Then() const { return *children_[1]; }

const Expression& CaseExpression::Else() const { return *children_[2]; }

}  // namespace kush::plan