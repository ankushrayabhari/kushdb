#include "compile/cpp/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "compile/cpp/cpp_compilation_context.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile::cpp {

ExpressionTranslator::ExpressionTranslator(CppCompilationContext& context,
                                           OperatorTranslator& source)
    : context_(context), source_(source) {}

void ExpressionTranslator::Produce(const plan::Expression& expr) {
  expr.Accept(*this);
}

void ExpressionTranslator::Visit(const plan::ArithmeticExpression& arith) {
  auto type = arith.OpType();

  using OpType = plan::ArithmeticOperatorType;
  const static absl::flat_hash_map<OpType, std::string> type_to_op{
      {OpType::ADD, "+"},
      {OpType::SUB, "-"},
      {OpType::MUL, "*"},
      {OpType::DIV, "/"},
  };

  auto& program = context_.Program();

  program.fout << "(";
  Produce(arith.LeftChild());
  program.fout << ")" << type_to_op.at(type) << "(";
  Produce(arith.RightChild());
  program.fout << ")";
}

void ExpressionTranslator::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

void ExpressionTranslator::Visit(const plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().GetValues();
  auto& program = context_.Program();
  program.fout << values.Variable(col_ref.GetColumnIdx());
}

void ExpressionTranslator::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.GetVirtualValues();
  auto& program = context_.Program();
  program.fout << values.Variable(col_ref.GetColumnIdx());
}

void ExpressionTranslator::Visit(const plan::ComparisonExpression& comp) {
  auto type = comp.CompType();

  using CompType = plan::ComparisonType;
  const static absl::flat_hash_map<CompType, std::string> type_to_op{
      {CompType::EQ, "=="},  {CompType::NEQ, "!="}, {CompType::LT, "<"},
      {CompType::LEQ, "<="}, {CompType::GT, ">"},   {CompType::GEQ, ">="},
      {CompType::AND, "&&"}, {CompType::OR, "||"}};

  auto& program = context_.Program();

  program.fout << "(";
  Produce(comp.LeftChild());
  program.fout << ")" << type_to_op.at(type) << "(";
  Produce(comp.RightChild());
  program.fout << ")";
}

void ExpressionTranslator::Visit(const plan::LiteralExpression& literal) {
  auto& program = context_.Program();
  switch (literal.Type()) {
    case catalog::SqlType::SMALLINT:
      program.fout << literal.GetSmallintValue();
      break;
    case catalog::SqlType::INT:
      program.fout << literal.GetIntValue();
      break;
    case catalog::SqlType::BIGINT:
      program.fout << literal.GetBigintValue();
      break;
    case catalog::SqlType::DATE:
      program.fout << literal.GetDateValue();
      break;
    case catalog::SqlType::REAL:
      program.fout << literal.GetRealValue();
      break;
    case catalog::SqlType::TEXT:
      program.fout << "\"" << literal.GetTextValue() << "\"";
      break;
    case catalog::SqlType::BOOLEAN:
      program.fout << literal.GetBooleanValue();
      break;
  }
}

}  // namespace kush::compile::cpp