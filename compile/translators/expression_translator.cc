#include "compile/translators/expression_translator.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
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

void ExpressionTranslator::Visit(const plan::UnaryArithmeticExpression& arith) {
  using OpType = plan::UnaryArithmeticExpressionType;
  switch (arith.OpType()) {
    case OpType::IS_NULL: {
      auto value = Compute(arith.Child());
      Return(proxy::SQLValue(value.IsNull(), proxy::Bool(program_, false)));
      return;
    }

    case OpType::NOT: {
      auto value = Compute(arith.Child());
      Return(proxy::NullableTernary<proxy::Bool>(
          program_, value.IsNull(),
          [&]() {
            return proxy::SQLValue(proxy::Bool(program_, false),
                                   proxy::Bool(program_, true));
          },
          [&]() {
            auto& boolean = dynamic_cast<proxy::Bool&>(value.Get());
            return proxy::SQLValue(!boolean, proxy::Bool(program_, false));
          }));
      return;
    }
  }
}

void ExpressionTranslator::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  using OpType = plan::BinaryArithmeticExpressionType;
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
      Return(EvaluateBinary(arith.OpType(), lhs, rhs));
  }
}

void ExpressionTranslator::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregate expression can't be derived");
}

void ExpressionTranslator::Visit(const plan::LiteralExpression& literal) {
  literal.Visit(
      [&](int16_t v, bool null) {
        Return(proxy::SQLValue(proxy::Int16(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](int32_t v, bool null) {
        Return(proxy::SQLValue(proxy::Int32(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](int64_t v, bool null) {
        Return(proxy::SQLValue(proxy::Int64(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](double v, bool null) {
        Return(proxy::SQLValue(proxy::Float64(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](std::string v, bool null) {
        Return(proxy::SQLValue(proxy::String::Global(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](bool v, bool null) {
        Return(proxy::SQLValue(proxy::Bool(program_, v),
                               proxy::Bool(program_, null)));
      },
      [&](absl::CivilDay v, bool null) {
        Return(proxy::SQLValue(proxy::Date(program_, v),
                               proxy::Bool(program_, null)));
      });
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
      Return(Ternary<proxy::Date>(case_expr));
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