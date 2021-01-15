#pragma once

#include <memory>
#include <vector>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/value.h"
#include "compile/translators/operator_translator.h"
#include "plan/expression/expression_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class ExpressionTranslator
    : public util::Visitor<const plan::Expression&,
                           plan::ImmutableExpressionVisitor,
                           std::unique_ptr<proxy::Value>> {
 public:
  ExpressionTranslator(CppCompilationContext& context,
                       OperatorTranslator& source);
  virtual ~ExpressionTranslator() = default;

  void Visit(const plan::AggregateExpression& agg) override;
  void Visit(const plan::ColumnRefExpression& col_ref) override;
  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override;
  void Visit(const plan::LiteralExpression& literal) override;
  void Visit(const plan::BinaryArithmeticExpression& arith) override;
  void Visit(const plan::CaseExpression& case_expr) override;

 private:
  CppCompilationContext& context_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile