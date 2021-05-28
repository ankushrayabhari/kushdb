#include "compile/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile {

ExpressionTranslator::ExpressionTranslator(khir::KHIRProgramBuilder& program,
                                           OperatorTranslator& source)
    : program_(program), source_(source) {}

void ExpressionTranslator::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  using OpType = plan::BinaryArithmeticOperatorType;
  switch (arith.OpType()) {
    // Special handling for AND/OR for short circuiting
    case OpType::AND: {
      std::unique_ptr<proxy::Bool> th, el;
      auto left_value = ComputeAs<proxy::Bool>(arith.LeftChild());

      auto ret_val = proxy::If(
          program_, left_value,
          [&]() -> std::vector<khir::Value> {
            return {Compute(arith.RightChild())->Get()};
          },
          [&]() -> std::vector<khir::Value> {
            return {proxy::Bool(program_, false).Get()};
          })[0];
      this->Return(proxy::Bool(program_, ret_val).ToPointer());
      break;
    }

    case OpType::OR: {
      std::unique_ptr<proxy::Bool> th, el;
      auto left_value = ComputeAs<proxy::Bool>(arith.LeftChild());
      auto ret_val = proxy::If(
          program_, left_value,
          [&]() -> std::vector<khir::Value> {
            return {proxy::Bool(program_, true).Get()};
          },
          [&]() -> std::vector<khir::Value> {
            return {Compute(arith.RightChild())->Get()};
          })[0];
      this->Return(proxy::Bool(program_, ret_val).ToPointer());
      break;
    }

    default:
      auto left_value = this->Compute(arith.LeftChild());
      auto right_value = this->Compute(arith.RightChild());
      this->Return(left_value->EvaluateBinary(arith.OpType(), *right_value));
      break;
  }
}

void ExpressionTranslator::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

void ExpressionTranslator::Visit(const plan::LiteralExpression& literal) {
  switch (literal.Type()) {
    case catalog::SqlType::SMALLINT:
      this->Return(
          std::make_unique<proxy::Int16>(program_, literal.GetSmallintValue()));
      break;
    case catalog::SqlType::INT:
      this->Return(
          std::make_unique<proxy::Int32>(program_, literal.GetIntValue()));
      break;
    case catalog::SqlType::BIGINT:
      this->Return(
          std::make_unique<proxy::Int64>(program_, literal.GetBigintValue()));
      break;
    case catalog::SqlType::DATE:
      this->Return(
          std::make_unique<proxy::Int64>(program_, literal.GetDateValue()));
      break;
    case catalog::SqlType::REAL:
      this->Return(
          std::make_unique<proxy::Float64>(program_, literal.GetRealValue()));
      break;
    case catalog::SqlType::TEXT:
      this->Return(proxy::String::Constant(program_, literal.GetTextValue())
                       .ToPointer());
      break;
    case catalog::SqlType::BOOLEAN:
      this->Return(
          std::make_unique<proxy::Bool>(program_, literal.GetBooleanValue()));
      break;
  }
}

std::unique_ptr<proxy::Value> CopyProxyValue(khir::KHIRProgramBuilder& program,
                                             catalog::SqlType type,
                                             const khir::Value& value) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return std::make_unique<proxy::Int16>(program, value);
    case catalog::SqlType::INT:
      return std::make_unique<proxy::Int32>(program, value);
    case catalog::SqlType::BIGINT:
      return std::make_unique<proxy::Int64>(program, value);
    case catalog::SqlType::DATE:
      return std::make_unique<proxy::Int64>(program, value);
    case catalog::SqlType::REAL:
      return std::make_unique<proxy::Float64>(program, value);
    case catalog::SqlType::TEXT:
      return std::make_unique<proxy::String>(program, value);
    case catalog::SqlType::BOOLEAN:
      return std::make_unique<proxy::Bool>(program, value);
  }
}

void ExpressionTranslator::Visit(const plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().SchemaValues();
  auto type = col_ref.Type();
  auto& value = values.Value(col_ref.GetColumnIdx());
  this->Return(CopyProxyValue(program_, type, value.Get()));
}

void ExpressionTranslator::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.VirtualSchemaValues();
  auto type = col_ref.Type();
  auto& value = values.Value(col_ref.GetColumnIdx());
  this->Return(CopyProxyValue(program_, type, value.Get()));
}

template <typename S>
std::unique_ptr<S> ExpressionTranslator::Ternary(
    const plan::CaseExpression& case_expr) {
  std::unique_ptr<S> th, el;
  auto cond = ComputeAs<proxy::Bool>(case_expr.Cond());
  auto ret_val = proxy::If(
      program_, cond,
      [&]() -> std::vector<khir::Value> {
        return {this->Compute(case_expr.Then())->Get()};
      },
      [&]() -> std::vector<khir::Value> {
        return {this->Compute(case_expr.Else())->Get()};
      })[0];
  return S(program_, ret_val).ToPointer();
}

void ExpressionTranslator::Visit(const plan::CaseExpression& case_expr) {
  switch (case_expr.Type()) {
    case catalog::SqlType::SMALLINT: {
      this->Return(Ternary<proxy::Int16>(case_expr));
      break;
    }
    case catalog::SqlType::INT: {
      this->Return(Ternary<proxy::Int32>(case_expr));
      break;
    }
    case catalog::SqlType::BIGINT: {
      this->Return(Ternary<proxy::Int64>(case_expr));
      break;
    }
    case catalog::SqlType::DATE: {
      this->Return(Ternary<proxy::Int64>(case_expr));
      break;
    }
    case catalog::SqlType::REAL: {
      this->Return(Ternary<proxy::Float64>(case_expr));
      break;
    }
    case catalog::SqlType::TEXT: {
      this->Return(Ternary<proxy::String>(case_expr));
      break;
    }
    case catalog::SqlType::BOOLEAN: {
      this->Return(Ternary<proxy::Bool>(case_expr));
      break;
    }
  }
}

void ExpressionTranslator::Visit(
    const plan::IntToFloatConversionExpression& conv_expr) {
  auto v = this->Compute(conv_expr.Child());

  if (auto i = dynamic_cast<proxy::Int8*>(v.get())) {
    this->Return(proxy::Float64(program_, *i).ToPointer());
    return;
  }

  if (auto i = dynamic_cast<proxy::Int16*>(v.get())) {
    this->Return(proxy::Float64(program_, *i).ToPointer());
    return;
  }

  if (auto i = dynamic_cast<proxy::Int32*>(v.get())) {
    this->Return(proxy::Float64(program_, *i).ToPointer());
    return;
  }

  if (auto i = dynamic_cast<proxy::Int64*>(v.get())) {
    this->Return(proxy::Float64(program_, *i).ToPointer());
    return;
  }

  throw std::runtime_error("Not an integer input.");
}

}  // namespace kush::compile