#include "compile/translators/select_translator.h"

#include "compile/proxy/control_flow/if.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/select_operator.h"

namespace kush::compile {

SelectTranslator::SelectTranslator(
    const plan::SelectOperator& select, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(select, std::move(children)),
      select_(select),
      program_(program),
      expr_translator_(program, *this) {}

void SelectTranslator::Produce() { this->Child().Produce(); }

void SelectTranslator::Consume(OperatorTranslator& src) {
  auto value = expr_translator_.template ComputeAs<proxy::Bool>(select_.Expr());

  proxy::If(program_, value, [&]() {
    this->values_.ResetValues();
    for (const auto& column : select_.Schema().Columns()) {
      this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  });
}

}  // namespace kush::compile