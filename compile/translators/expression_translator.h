#pragma once

#include <memory>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

class ExpressionTranslator : public plan::ImmutableExpressionVisitor {
 public:
  ExpressionTranslator(CompilationContext& context, OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  void Produce(const plan::Expression& expr);
  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::ComparisonExpression& comp) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::ArithmeticExpression& arith) override;

 private:
  CompilationContext& context_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile