#include "compilation/translators/expression_translator.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "compilation/cpp_translator.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"

namespace kush::compile {

ExpressionTranslator::ExpressionTranslator(CppTranslator& context,
                                           OperatorTranslator& source)
    : context_(context), source_(source) {}

void ExpressionTranslator::Produce(plan::Expression& expr) {
  expr.Accept(*this);
}

void ExpressionTranslator::Visit(plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().GetValues();
  auto var = values.Get(col_ref.GetColumnIdx());
  Return(var);
}

void ExpressionTranslator::Visit(plan::ComparisonExpression& comp) {
  comp.Left().Accept(*this);
  auto l = GetResult();

  comp.Right().Accept(*this);
  auto r = GetResult();

  auto type = comp.Type();

  using CompType = plan::ComparisonType;
  std::unordered_map<CompType, std::string> type_to_op{
      {CompType::EQ, "=="},  {CompType::NEQ, "!="}, {CompType::LT, "<"},
      {CompType::LEQ, "<="}, {CompType::GT, ">"},   {CompType::GEQ, ">="},
  };

  std::string value;
  value.append("(");
  value.append(l);
  value.append(")");
  value.append(type_to_op.at(type));
  value.append("(");
  value.append(r);
  value.append(")");
  Return(std::move(value));
}

void ExpressionTranslator::Visit(plan::LiteralExpression<int32_t>& literal) {
  Return(std::to_string(literal.GetValue()));
}

}  // namespace kush::compile