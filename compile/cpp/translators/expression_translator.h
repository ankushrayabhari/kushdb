#pragma once

#include <memory>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

class ExpressionTranslator : public plan::ImmutableExpressionVisitor {
 public:
  ExpressionTranslator(CppCompilationContext& context,
                       OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  void Produce(const plan::Expression& expr);
  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::ComparisonExpression& comp) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::ArithmeticExpression& arith) override;

 private:
  CppCompilationContext& context_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile