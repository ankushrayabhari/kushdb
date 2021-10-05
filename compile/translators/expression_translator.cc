#include "compile/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/extract_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "runtime/date_extractor.h"

namespace kush::compile {

constexpr std::string_view ExtractYearFnName(
    "kush::runtime::DateExtractor::ExtractYear");

void ExpressionTranslator::ForwardDeclare(khir::ProgramBuilder& program) {
  auto date_type = program.I64Type();
  auto result_type = program.I32Type();

  program.DeclareExternalFunction(
      ExtractYearFnName, result_type, {date_type},
      reinterpret_cast<void*>(&runtime::DateExtractor::ExtractYear));
}

ExpressionTranslator::ExpressionTranslator(khir::ProgramBuilder& program,
                                           OperatorTranslator& source)
    : program_(program), source_(source) {}

proxy::SQLValue ExpressionTranslator::EvaluateBinaryBool(
    plan::BinaryArithmeticOperatorType op_type, const proxy::SQLValue& lhs,
    const proxy::SQLValue& rhs) {
  return proxy::NullableTernary<proxy::Bool>(
      program_, lhs.IsNull() || rhs.IsNull(),
      [&]() {
        return proxy::SQLValue(proxy::Bool(program_, false),
                               proxy::Bool(program_, true));
      },
      [&]() {
        const proxy::Bool& lhs_bool =
            dynamic_cast<const proxy::Bool&>(lhs.Get());
        const proxy::Bool& rhs_bool =
            dynamic_cast<const proxy::Bool&>(rhs.Get());
        proxy::Bool null(program_, false);

        switch (op_type) {
          case plan::BinaryArithmeticOperatorType::EQ:
            return proxy::SQLValue(lhs_bool == rhs_bool, null);

          case plan::BinaryArithmeticOperatorType::NEQ:
            return proxy::SQLValue(lhs_bool != rhs_bool, null);

          default:
            throw std::runtime_error("Invalid operator on Bool");
        }
      });
}

template <typename V>
proxy::SQLValue ExpressionTranslator::EvaluateBinaryNumeric(
    plan::BinaryArithmeticOperatorType op_type, const proxy::SQLValue& lhs,
    const proxy::SQLValue& rhs) {
  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::ADD:
    case plan::BinaryArithmeticOperatorType::SUB:
    case plan::BinaryArithmeticOperatorType::MUL:
    case plan::BinaryArithmeticOperatorType::DIV: {
      return proxy::NullableTernary<proxy::Bool>(
          program_, lhs.IsNull() || rhs.IsNull(),
          [&]() {
            return proxy::SQLValue(proxy::Bool(program_, false),
                                   proxy::Bool(program_, true));
          },
          [&]() {
            const V& lhs_v = dynamic_cast<const V&>(lhs.Get());
            const V& rhs_v = dynamic_cast<const V&>(rhs.Get());
            proxy::Bool null(program_, false);

            switch (op_type) {
              case plan::BinaryArithmeticOperatorType::ADD:
                return proxy::SQLValue(lhs_v + rhs_v, null);

              case plan::BinaryArithmeticOperatorType::SUB:
                return proxy::SQLValue(lhs_v - rhs_v, null);

              case plan::BinaryArithmeticOperatorType::MUL:
                return proxy::SQLValue(lhs_v * rhs_v, null);

              case plan::BinaryArithmeticOperatorType::DIV:
                if constexpr (std::is_same_v<V, proxy::Float64>) {
                  return proxy::SQLValue(lhs_v / rhs_v, null);
                } else {
                  throw std::runtime_error("Invalid operator on numeric");
                }

              default:
                throw std::runtime_error("Unreachable");
            }
          });
    }

    case plan::BinaryArithmeticOperatorType::EQ:
    case plan::BinaryArithmeticOperatorType::NEQ:
    case plan::BinaryArithmeticOperatorType::LT:
    case plan::BinaryArithmeticOperatorType::LEQ:
    case plan::BinaryArithmeticOperatorType::GT:
    case plan::BinaryArithmeticOperatorType::GEQ: {
      return proxy::NullableTernary<proxy::Bool>(
          program_, lhs.IsNull() || rhs.IsNull(),
          [&]() {
            return proxy::SQLValue(proxy::Bool(program_, false),
                                   proxy::Bool(program_, true));
          },
          [&]() {
            const V& lhs_v = dynamic_cast<const V&>(lhs.Get());
            const V& rhs_v = dynamic_cast<const V&>(rhs.Get());
            proxy::Bool null(program_, false);

            switch (op_type) {
              case plan::BinaryArithmeticOperatorType::EQ:
                return proxy::SQLValue(lhs_v == rhs_v, null);

              case plan::BinaryArithmeticOperatorType::NEQ:
                return proxy::SQLValue(lhs_v != rhs_v, null);

              case plan::BinaryArithmeticOperatorType::LT:
                return proxy::SQLValue(lhs_v < rhs_v, null);

              case plan::BinaryArithmeticOperatorType::LEQ:
                return proxy::SQLValue(lhs_v <= rhs_v, null);

              case plan::BinaryArithmeticOperatorType::GT:
                return proxy::SQLValue(lhs_v > rhs_v, null);

              case plan::BinaryArithmeticOperatorType::GEQ:
                return proxy::SQLValue(lhs_v >= rhs_v, null);

              default:
                throw std::runtime_error("Unreachable");
            }
          });
    }

    default:
      throw std::runtime_error("Invalid operator on numeric");
  }
}

proxy::SQLValue ExpressionTranslator::EvaluateBinaryString(
    plan::BinaryArithmeticOperatorType op_type, const proxy::SQLValue& lhs,
    const proxy::SQLValue& rhs) {
  return proxy::NullableTernary<proxy::Bool>(
      program_, lhs.IsNull() || rhs.IsNull(),
      [&]() {
        return proxy::SQLValue(proxy::Bool(program_, false),
                               proxy::Bool(program_, true));
      },
      [&]() {
        const proxy::String& lhs_v =
            dynamic_cast<const proxy::String&>(lhs.Get());
        const proxy::String& rhs_v =
            dynamic_cast<const proxy::String&>(rhs.Get());
        proxy::Bool null(program_, false);

        switch (op_type) {
          case plan::BinaryArithmeticOperatorType::STARTS_WITH:
            return proxy::SQLValue(lhs_v.StartsWith(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::ENDS_WITH:
            return proxy::SQLValue(lhs_v.EndsWith(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::CONTAINS:
            return proxy::SQLValue(lhs_v.Contains(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::LIKE:
            return proxy::SQLValue(lhs_v.Like(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::EQ:
            return proxy::SQLValue(lhs_v == rhs_v, null);

          case plan::BinaryArithmeticOperatorType::NEQ:
            return proxy::SQLValue(lhs_v != rhs_v, null);

          case plan::BinaryArithmeticOperatorType::LT:
            return proxy::SQLValue(lhs_v < rhs_v, null);

          default:
            throw std::runtime_error("Invalid operator on string");
        }
      });
}

void ExpressionTranslator::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  using OpType = plan::BinaryArithmeticOperatorType;
  switch (arith.OpType()) {
    // Special handling for AND/OR for short circuiting
    case OpType::AND: {
      auto left_value = Compute(arith.LeftChild());
      Return(proxy::NullableTernary<proxy::Bool>(
          program_, left_value.IsNull(),
          [&]() {
            return proxy::SQLValue(proxy::Bool(program_, false),
                                   proxy::Bool(program_, true));
          },
          [&]() {
            return proxy::NullableTernary<proxy::Bool>(
                program_, dynamic_cast<proxy::Bool&>(left_value.Get()),
                [&]() { return Compute(arith.RightChild()); },
                [&]() {
                  return proxy::SQLValue(proxy::Bool(program_, false),
                                         proxy::Bool(program_, false));
                });
          }));
      return;
    }

    case OpType::OR: {
      auto left_value = Compute(arith.LeftChild());
      Return(proxy::NullableTernary<proxy::Bool>(
          program_, left_value.IsNull(),
          [&]() { return Compute(arith.RightChild()); },
          [&]() {
            return proxy::NullableTernary<proxy::Bool>(
                program_, dynamic_cast<proxy::Bool&>(left_value.Get()),
                [&]() {
                  return proxy::SQLValue(proxy::Bool(program_, true),
                                         proxy::Bool(program_, false));
                },
                [&]() { return Compute(arith.RightChild()); });
          }));
      return;
    }

    default:
      auto lhs = Compute(arith.LeftChild());
      auto rhs = Compute(arith.RightChild());

      switch (lhs.Type()) {
        case catalog::SqlType::BOOLEAN:
          Return(EvaluateBinaryBool(arith.OpType(), lhs, rhs));
          return;
        case catalog::SqlType::SMALLINT:
          Return(EvaluateBinaryNumeric<proxy::Int16>(arith.OpType(), lhs, rhs));
          return;
        case catalog::SqlType::INT:
          Return(EvaluateBinaryNumeric<proxy::Int32>(arith.OpType(), lhs, rhs));
          return;
        case catalog::SqlType::DATE:
        case catalog::SqlType::BIGINT:
          Return(EvaluateBinaryNumeric<proxy::Int64>(arith.OpType(), lhs, rhs));
          return;
        case catalog::SqlType::REAL:
          Return(
              EvaluateBinaryNumeric<proxy::Float64>(arith.OpType(), lhs, rhs));
          return;
        case catalog::SqlType::TEXT:
          Return(EvaluateBinaryString(arith.OpType(), lhs, rhs));
          return;
      }
  }
}

void ExpressionTranslator::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

void ExpressionTranslator::Visit(const plan::LiteralExpression& literal) {
  proxy::Bool null(program_, false);
  switch (literal.Type()) {
    case catalog::SqlType::SMALLINT:
      Return(proxy::SQLValue(proxy::Int16(program_, literal.GetSmallintValue()),
                             null));
      return;
    case catalog::SqlType::INT:
      Return(
          proxy::SQLValue(proxy::Int32(program_, literal.GetIntValue()), null));
      return;
    case catalog::SqlType::BIGINT:
      Return(proxy::SQLValue(proxy::Int64(program_, literal.GetBigintValue()),
                             null));
      return;
    case catalog::SqlType::DATE:
      Return(proxy::SQLValue(proxy::Int64(program_, literal.GetDateValue()),
                             null));
      return;
    case catalog::SqlType::REAL:
      Return(proxy::SQLValue(proxy::Float64(program_, literal.GetRealValue()),
                             null));
      return;
    case catalog::SqlType::TEXT:
      Return(proxy::SQLValue(
          proxy::String::Global(program_, literal.GetTextValue()), null));
      return;
    case catalog::SqlType::BOOLEAN:
      Return(proxy::SQLValue(proxy::Bool(program_, literal.GetBooleanValue()),
                             null));
      return;
  }
}

void ExpressionTranslator::Visit(const plan::ColumnRefExpression& col_ref) {
  auto& values = source_.Children()[col_ref.GetChildIdx()].get().SchemaValues();
  Return(values.Value(col_ref.GetColumnIdx()));
}

void ExpressionTranslator::Visit(
    const plan::VirtualColumnRefExpression& col_ref) {
  auto& values = source_.VirtualSchemaValues();
  Return(values.Value(col_ref.GetColumnIdx()));
}

template <typename S>
proxy::SQLValue ExpressionTranslator::Ternary(
    const plan::CaseExpression& case_expr) {
  auto cond = Compute(case_expr.Cond());
  return proxy::NullableTernary<S>(
      program_, cond.IsNull(), [&]() { return Compute(case_expr.Else()); },
      [&]() {
        return proxy::NullableTernary<S>(
            program_, static_cast<proxy::Bool&>(cond.Get()),
            [&]() { return Compute(case_expr.Then()); },
            [&]() { return Compute(case_expr.Else()); });
      });
}

void ExpressionTranslator::Visit(const plan::CaseExpression& case_expr) {
  switch (case_expr.Type()) {
    case catalog::SqlType::SMALLINT: {
      Return(Ternary<proxy::Int16>(case_expr));
      return;
    }
    case catalog::SqlType::INT: {
      Return(Ternary<proxy::Int32>(case_expr));
      return;
    }
    case catalog::SqlType::BIGINT: {
      Return(Ternary<proxy::Int64>(case_expr));
      return;
    }
    case catalog::SqlType::DATE: {
      Return(Ternary<proxy::Int64>(case_expr));
      return;
    }
    case catalog::SqlType::REAL: {
      Return(Ternary<proxy::Float64>(case_expr));
      return;
    }
    case catalog::SqlType::TEXT: {
      Return(Ternary<proxy::String>(case_expr));
      return;
    }
    case catalog::SqlType::BOOLEAN: {
      Return(Ternary<proxy::Bool>(case_expr));
      return;
    }
  }
}

void ExpressionTranslator::Visit(
    const plan::IntToFloatConversionExpression& conv_expr) {
  auto v = Compute(conv_expr.Child());

  Return(proxy::NullableTernary<proxy::Float64>(
      program_, v.IsNull(),
      [&]() {
        return proxy::SQLValue(proxy::Float64(program_, 0),
                               proxy::Bool(program_, false));
      },
      [&]() {
        if (auto i = dynamic_cast<proxy::Int8*>(&v.Get())) {
          return proxy::SQLValue(proxy::Float64(program_, *i),
                                 proxy::Bool(program_, false));
        }

        if (auto i = dynamic_cast<proxy::Int16*>(&v.Get())) {
          return proxy::SQLValue(proxy::Float64(program_, *i),
                                 proxy::Bool(program_, false));
        }

        if (auto i = dynamic_cast<proxy::Int32*>(&v.Get())) {
          return proxy::SQLValue(proxy::Float64(program_, *i),
                                 proxy::Bool(program_, false));
        }

        if (auto i = dynamic_cast<proxy::Int64*>(&v.Get())) {
          return proxy::SQLValue(proxy::Float64(program_, *i),
                                 proxy::Bool(program_, false));
        }

        throw std::runtime_error("Not an integer input.");
      }));
}

void ExpressionTranslator::Visit(const plan::ExtractExpression& extract_expr) {
  auto v = Compute(extract_expr.Child());

  switch (extract_expr.ValueToExtract()) {
    case plan::ExtractValue::YEAR:
      Return(proxy::NullableTernary<proxy::Int32>(
          program_, v.IsNull(),
          [&]() {
            return proxy::SQLValue(proxy::Int32(program_, 0),
                                   proxy::Bool(program_, true));
          },
          [&]() {
            return proxy::SQLValue(
                proxy::Int32(program_, program_.Call(program_.GetFunction(
                                                         ExtractYearFnName),
                                                     {v.Get().Get()})),
                proxy::Bool(program_, false));
          }));
      return;
  }
}

}  // namespace kush::compile