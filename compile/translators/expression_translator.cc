#include "compile/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile {

template <typename T>
ExpressionTranslator<T>::ExpressionTranslator(ProgramBuilder<T>& program,
                                              OperatorTranslator<T>& source)
    : program_(program), source_(source) {}

template <typename T>
void ExpressionTranslator<T>::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  using OpType = plan::BinaryArithmeticOperatorType;
  switch (arith.OpType()) {
    // Special handling for AND/OR for short circuiting
    case OpType::AND: {
      std::unique_ptr<proxy::Value<T>> th, el;
      auto left_value = this->Compute(arith.LeftChild());
      proxy::If branch(
          program_, dynamic_cast<proxy::Bool<T>&>(*left_value),
          [&]() { th = this->Compute(arith.RightChild()); },
          [&]() { el = std::make_unique<proxy::Bool<T>>(program_, false); });

      proxy::Bool<T>& th_ref = dynamic_cast<proxy::Bool<T>&>(*th);
      proxy::Bool<T>& el_ref = dynamic_cast<proxy::Bool<T>&>(*el);

      this->Return(std::make_unique<proxy::Bool<T>>(
          program_, branch.Phi(th_ref, el_ref)));
      break;
    }

    case OpType::OR: {
      std::unique_ptr<proxy::Value<T>> th, el;
      auto left_value = this->Compute(arith.LeftChild());
      proxy::If branch(
          program_, dynamic_cast<proxy::Bool<T>&>(*left_value),
          [&]() { th = std::make_unique<proxy::Bool<T>>(program_, true); },
          [&]() { el = this->Compute(arith.RightChild()); });

      proxy::Bool<T>& th_ref = dynamic_cast<proxy::Bool<T>&>(*th);
      proxy::Bool<T>& el_ref = dynamic_cast<proxy::Bool<T>&>(*el);

      this->Return(std::make_unique<proxy::Bool<T>>(
          program_, branch.Phi(th_ref, el_ref)));
      break;
    }

    default:
      auto left_value = this->Compute(arith.LeftChild());
      auto right_value = this->Compute(arith.RightChild());
      this->Return(left_value->EvaluateBinary(arith.OpType(), *right_value));
      break;
  }
}

template <typename T>
void ExpressionTranslator<T>::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

template <typename T>
void ExpressionTranslator<T>::Visit(const plan::LiteralExpression& literal) {
  switch (literal.Type()) {
    case catalog::SqlType::SMALLINT:
      this->Return(std::make_unique<proxy::Int16<T>>(
          program_, literal.GetSmallintValue()));
      break;
    case catalog::SqlType::INT:
      this->Return(
          std::make_unique<proxy::Int32<T>>(program_, literal.GetIntValue()));
      break;
    case catalog::SqlType::BIGINT:
      this->Return(std::make_unique<proxy::Int64<T>>(program_,
                                                     literal.GetBigintValue()));
      break;
    case catalog::SqlType::DATE:
      this->Return(
          std::make_unique<proxy::Int64<T>>(program_, literal.GetDateValue()));
      break;
    case catalog::SqlType::REAL:
      this->Return(std::make_unique<proxy::Float64<T>>(program_,
                                                       literal.GetRealValue()));
      break;
    case catalog::SqlType::TEXT:
      this->Return(std::make_unique<proxy::String<T>>(
          program_,
          proxy::String<T>::Constant(program_, literal.GetTextValue())));
      break;
    case catalog::SqlType::BOOLEAN:
      this->Return(std::make_unique<proxy::Bool<T>>(program_,
                                                    literal.GetBooleanValue()));
      break;
  }
}

template <typename T>
std::unique_ptr<proxy::Value<T>> CopyProxyValue(
    ProgramBuilder<T>& program, catalog::SqlType type,
    typename ProgramBuilder<T>::Value& value) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return std::make_unique<proxy::Int16<T>>(program, value);
    case catalog::SqlType::INT:
      return std::make_unique<proxy::Int32<T>>(program, value);
    case catalog::SqlType::BIGINT:
      return std::make_unique<proxy::Int64<T>>(program, value);
    case catalog::SqlType::DATE:
      return std::make_unique<proxy::Int64<T>>(program, value);
    case catalog::SqlType::REAL:
      return std::make_unique<proxy::Float64<T>>(program, value);
    case catalog::SqlType::TEXT:
      return std::make_unique<proxy::String<T>>(program, value);
    case catalog::SqlType::BOOLEAN:
      return std::make_unique<proxy::Bool<T>>(program, value);
  }
}

template <typename T>
void ExpressionTranslator<T>::Visit(const plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().SchemaValues();
  auto type = col_ref.Type();
  auto& value = values.Value(col_ref.GetColumnIdx());
  this->Return(CopyProxyValue(program_, type, value.Get()));
}

template <typename T>
void ExpressionTranslator<T>::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.VirtualSchemaValues();
  auto type = col_ref.Type();
  auto& value = values.Value(col_ref.GetColumnIdx());
  this->Return(CopyProxyValue(program_, type, value.Get()));
}

template <typename S, typename T>
std::unique_ptr<S> Ternary(ProgramBuilder<T>& program,
                           ExpressionTranslator<T>& translator,
                           const plan::CaseExpression& case_expr) {
  std::unique_ptr<proxy::Value<T>> th, el;
  auto cond = translator.Compute(case_expr.Cond());
  proxy::If branch(
      program, dynamic_cast<proxy::Bool<T>&>(*cond),
      [&]() { th = translator.Compute(case_expr.Then()); },
      [&]() { el = translator.Compute(case_expr.Else()); });

  S& th_ref = dynamic_cast<S&>(*th);
  S& el_ref = dynamic_cast<S&>(*el);

  return std::make_unique<S>(program, branch.Phi(th_ref, el_ref));
}

template <typename T>
void ExpressionTranslator<T>::Visit(const plan::CaseExpression& case_expr) {
  switch (case_expr.Type()) {
    case catalog::SqlType::SMALLINT: {
      this->Return(Ternary<proxy::Int16<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::INT: {
      this->Return(Ternary<proxy::Int32<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::BIGINT: {
      this->Return(Ternary<proxy::Int64<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::DATE: {
      this->Return(Ternary<proxy::Int64<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::REAL: {
      this->Return(Ternary<proxy::Float64<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::TEXT: {
      this->Return(Ternary<proxy::String<T>, T>(program_, *this, case_expr));
      break;
    }
    case catalog::SqlType::BOOLEAN: {
      this->Return(Ternary<proxy::Bool<T>, T>(program_, *this, case_expr));
      break;
    }
  }
}

INSTANTIATE_ON_IR(ExpressionTranslator);

}  // namespace kush::compile