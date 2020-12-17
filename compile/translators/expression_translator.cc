#include "compile/translators/expression_translator.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "compile/compilation_context.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"

namespace kush::compile {

ExpressionTranslator::ExpressionTranslator(CompilationContext& context,
                                           OperatorTranslator& source)
    : context_(context), source_(source) {}

void ExpressionTranslator::Produce(plan::Expression& expr) {
  expr.Accept(*this);
}

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
  Produce(comp.Left());
  program.fout << ")" << type_to_op.at(type) << "(";
  Produce(comp.Right());
  program.fout << ")";
}

void ExpressionTranslator::Visit(plan::LiteralExpression& literal) {
  auto& program = context_.Program();
  program.fout << literal.GetValue();
}

}  // namespace kush::compile