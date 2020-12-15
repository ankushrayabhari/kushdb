#include "compilation/translators/expression_translator.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "compilation/compilation_context.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"

namespace kush::compile {

ExpressionTranslator::ExpressionTranslator(CompliationContext& context,
                                           OperatorTranslator& source)
    : context_(context), source_(source) {}

void ExpressionTranslator::Visit(plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().GetValues();
  auto& program = context_.Program();
  program.fout << values.Variable(col_ref.GetColumnIdx());
}

void ExpressionTranslator::Visit(plan::ComparisonExpression& comp) {
  auto type = comp.Type();

  using CompType = plan::ComparisonType;
  const std::unordered_map<CompType, std::string> type_to_op{
      {CompType::EQ, "=="},  {CompType::NEQ, "!="}, {CompType::LT, "<"},
      {CompType::LEQ, "<="}, {CompType::GT, ">"},   {CompType::GEQ, ">="},
  };

  auto& program = context_.Program();

  program.fout << "(";
  comp.Left().Accept(*this);
  program.fout << ")" << type_to_op.at(type) << "(";
  comp.Right().Accept(*this);
  program.fout << ")";
}

void ExpressionTranslator::Visit(plan::LiteralExpression<int32_t>& literal) {
  auto& program = context_.Program();
  program.fout << literal.GetValue();
}

}  // namespace kush::compile