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
  program.fout << values.Variable(col_ref.GetColumnIdx());
}

void ExpressionTranslator::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.GetVirtualValues();
  auto& program = context_.Program();
  program.fout << values.Variable(col_ref.GetColumnIdx());
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

void ExpressionTranslator::Visit(const plan::CaseExpression& case_expr) {
  auto& program = context_.Program();

  auto cond = Compute(case_expr.Cond());

  switch (case_expr.Type()) {
    case catalog::SqlType::SMALLINT: {
      auto result = std::make_unique<proxy::Int16>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::Int16&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::Int16&>(*el);
          });
      Return(std::move(result));
      break;
    }
    case catalog::SqlType::INT: {
      auto result = std::make_unique<proxy::Int32>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::Int32&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::Int32&>(*el);
          });
      Return(std::move(result));
      break;
    }
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE: {
      auto result = std::make_unique<proxy::Int64>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::Int64&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::Int64&>(*el);
          });
      Return(std::move(result));
      break;
    }
    case catalog::SqlType::REAL: {
      auto result = std::make_unique<proxy::Double>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::Double&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::Double&>(*el);
          });
      Return(std::move(result));
      break;
    }
    case catalog::SqlType::TEXT: {
      auto result = std::make_unique<proxy::StringView>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::StringView&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::StringView&>(*el);
          });
      Return(std::move(result));
      break;
    }
    case catalog::SqlType::BOOLEAN: {
      auto result = std::make_unique<proxy::Boolean>(program);
      codegen::If ternary(
          program, dynamic_cast<proxy::Boolean&>(*cond),
          [&]() {
            auto th = Compute(case_expr.Then());
            *result = dynamic_cast<proxy::Boolean&>(*th);
          },
          [&]() {
            auto el = Compute(case_expr.Else());
            *result = dynamic_cast<proxy::Boolean&>(*el);
          });
      Return(std::move(result));
      break;
    }
  }
}

}  // namespace kush::compile::cpp