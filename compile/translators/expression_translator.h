#pragma once

#include <memory>
#include <vector>

#include "compile/proxy/value/sql_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class ExpressionTranslator
    : public util::Visitor<plan::ImmutableExpressionVisitor,
                           const plan::Expression&, proxy::SQLValue> {
 public:
  ExpressionTranslator(khir::ProgramBuilder& program_,
                       OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::BinaryArithmeticExpression& arith) override;
  void Visit(const plan::CaseExpression& case_expr) override;
  void Visit(const plan::IntToFloatConversionExpression& conv_expr) override;
  void Visit(const plan::ExtractExpression& extract_expr) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  proxy::SQLValue EvaluateBinaryBool(plan::BinaryArithmeticOperatorType op_type,
                                     const proxy::SQLValue& lhs,
                                     const proxy::SQLValue& rhs);
  proxy::SQLValue EvaluateBinaryString(
      plan::BinaryArithmeticOperatorType op_type, const proxy::SQLValue& lhs,
      const proxy::SQLValue& rhs);
  template <typename V>
  proxy::SQLValue EvaluateBinaryNumeric(
      plan::BinaryArithmeticOperatorType op_type, const proxy::SQLValue& lhs,
      const proxy::SQLValue& rhs_generic);
  template <typename S>
  proxy::SQLValue Ternary(const plan::CaseExpression& case_expr);

  khir::ProgramBuilder& program_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile