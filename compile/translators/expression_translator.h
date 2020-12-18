#pragma once

#include <memory>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

class ExpressionTranslator : public plan::ExpressionVisitor {
 public:
  ExpressionTranslator(CompilationContext& context, OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  void Produce(plan::Expression& expr);
  void Visit(plan::AggregateExpression& agg) override;
  void Visit(plan::ColumnRefExpression& col_ref) override;
  void Visit(plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(plan::ComparisonExpression& comp) override;
  void Visit(plan::LiteralExpression& literal) override;

 private:
  CompilationContext& context_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile