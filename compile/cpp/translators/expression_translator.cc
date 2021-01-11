#include "compile/cpp/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "compile/cpp/codegen/if.h"
#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/proxy/boolean.h"
#include "compile/cpp/proxy/double.h"
#include "compile/cpp/proxy/integer.h"
#include "compile/cpp/proxy/string_view.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile::cpp {

ExpressionTranslator::ExpressionTranslator(CppCompilationContext& context,
                                           OperatorTranslator& source)
    : context_(context), source_(source) {}

void ExpressionTranslator::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  auto left_value = Compute(arith.LeftChild());
  auto right_value = Compute(arith.RightChild());
  Return(left_value->EvaluateBinary(arith.OpType(), *right_value));
}

void ExpressionTranslator::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

void ExpressionTranslator::Visit(const plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().GetValues();
  auto& program = context_.Program();

  switch (col_ref.Type()) {
    case catalog::SqlType::SMALLINT:
      Return(
          std::make_unique<proxy::Int16>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::INT:
      Return(
          std::make_unique<proxy::Int32>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE:
      Return(
          std::make_unique<proxy::Int64>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::REAL:
      Return(std::make_unique<proxy::Double>(
          values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::TEXT:
      Return(std::make_unique<proxy::StringView>(
          values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::BOOLEAN:
      Return(std::make_unique<proxy::Boolean>(
          values.Value(col_ref.GetColumnIdx())));
      break;
  }
}

void ExpressionTranslator::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.GetVirtualValues();
  auto& program = context_.Program();
  switch (col_ref.Type()) {
    case catalog::SqlType::SMALLINT:
      Return(
          std::make_unique<proxy::Int16>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::INT:
      Return(
          std::make_unique<proxy::Int32>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE:
      Return(
          std::make_unique<proxy::Int64>(values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::REAL:
      Return(std::make_unique<proxy::Double>(
          values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::TEXT:
      Return(std::make_unique<proxy::StringView>(
          values.Value(col_ref.GetColumnIdx())));
      break;
    case catalog::SqlType::BOOLEAN:
      Return(std::make_unique<proxy::Boolean>(
          values.Value(col_ref.GetColumnIdx())));
      break;
  }
}

void ExpressionTranslator::Visit(const plan::LiteralExpression& literal) {
  auto& program = context_.Program();
  switch (literal.Type()) {
    case catalog::SqlType::SMALLINT:
      Return(
          std::make_unique<proxy::Int16>(program, literal.GetSmallintValue()));
      break;
    case catalog::SqlType::INT:
      Return(std::make_unique<proxy::Int32>(program, literal.GetIntValue()));
      break;
    case catalog::SqlType::BIGINT:
      Return(std::make_unique<proxy::Int64>(program, literal.GetBigintValue()));
      break;
    case catalog::SqlType::DATE:
      Return(std::make_unique<proxy::Int64>(program, literal.GetDateValue()));
      break;
    case catalog::SqlType::REAL:
      Return(std::make_unique<proxy::Double>(program, literal.GetRealValue()));
      break;
    case catalog::SqlType::TEXT:
      Return(
          std::make_unique<proxy::StringView>(program, literal.GetTextValue()));
      break;
    case catalog::SqlType::BOOLEAN:
      Return(
          std::make_unique<proxy::Boolean>(program, literal.GetBooleanValue()));
      break;
  }
}

template <typename T>
std::unique_ptr<proxy::Value> Ternary(CppProgram& program,
                                      ExpressionTranslator& translator,
                                      const plan::CaseExpression& case_expr) {
  auto result = std::make_unique<T>(program);
  auto cond = translator.Compute(case_expr.Cond());
  codegen::If(
      program, dynamic_cast<proxy::Boolean&>(*cond),
      [&]() {
        auto th = translator.Compute(case_expr.Then());
        result->Assign(dynamic_cast<T&>(*th));
      },
      [&]() {
        auto el = translator.Compute(case_expr.Else());
        result->Assign(dynamic_cast<T&>(*el));
      });
  return std::move(result);
}

void ExpressionTranslator::Visit(const plan::CaseExpression& case_expr) {
  auto& program = context_.Program();

  switch (case_expr.Type()) {
    case catalog::SqlType::SMALLINT: {
      Return(Ternary<proxy::Int16>(program, *this, case_expr));
      break;
    }
    case catalog::SqlType::INT: {
      Return(Ternary<proxy::Int32>(program, *this, case_expr));
      break;
    }
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE: {
      Return(Ternary<proxy::Int64>(program, *this, case_expr));
      break;
    }
    case catalog::SqlType::REAL: {
      Return(Ternary<proxy::Double>(program, *this, case_expr));
      break;
    }
    case catalog::SqlType::TEXT: {
      Return(Ternary<proxy::StringView>(program, *this, case_expr));
      break;
    }
    case catalog::SqlType::BOOLEAN: {
      Return(Ternary<proxy::Boolean>(program, *this, case_expr));
      break;
    }
  }
}

}  // namespace kush::compile::cpp