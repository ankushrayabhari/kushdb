#pragma once

#include <memory>
#include <vector>

#include "compilation/compilation_context.h"
#include "compilation/translators/operator_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

class ExpressionTranslator : public plan::ExpressionVisitor {
 public:
  ExpressionTranslator(CompliationContext& context, OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  std::string Produce(plan::Expression& expr);
  void Visit(plan::ColumnRefExpression& col_ref) override;
  void Visit(plan::ComparisonExpression& comp) override;
  void Visit(plan::LiteralExpression<int32_t>& literal) override;

 private:
  CompliationContext& context_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile