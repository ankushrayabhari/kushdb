#pragma once

#include <memory>
#include <vector>

#include "compile/proxy/value/value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class ExpressionTranslator
    : public util::Visitor<plan::ImmutableExpressionVisitor,
                           const plan::Expression&, proxy::IRValue> {
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

  template <typename S>
  S ComputeAs(const plan::Expression& e) {
    auto p = this->Compute(e);
    if (S* result = dynamic_cast<S*>(p.get())) {
      return S(*result);
    }
    throw std::runtime_error("Invalid type.");
  }

 private:
  template <typename S>
  std::unique_ptr<S> Ternary(const plan::CaseExpression& case_expr);

  khir::ProgramBuilder& program_;
  OperatorTranslator& source_;
};

}  // namespace kush::compile