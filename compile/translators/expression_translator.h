#pragma once

#include <memory>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/value.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

template <typename T>
class ExpressionTranslator
    : public util::Visitor<plan::ImmutableExpressionVisitor,
                           const plan::Expression&, proxy::Value<T>> {
 public:
  ExpressionTranslator(ProgramBuilder<T>& program_,
                       OperatorTranslator<T>& source);
  virtual ~ExpressionTranslator() = default;

  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::BinaryArithmeticExpression& arith) override;
  void Visit(const plan::CaseExpression& case_expr) override;
  void Visit(const plan::IntToFloatConversionExpression& conv_expr) override;

 private:
  template <typename S>
  std::unique_ptr<S> ComputeAs(const plan::Expression&);

  template <typename S>
  std::unique_ptr<S> Ternary(const plan::CaseExpression& case_expr);

  ProgramBuilder<T>& program_;
  OperatorTranslator<T>& source_;
};

}  // namespace kush::compile