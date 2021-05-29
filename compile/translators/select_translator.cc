#include "compile/translators/select_translator.h"

#include "compile/khir/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
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

  proxy::If(
      program_, value,
      [&]() -> std::vector<khir::Value> {
        this->values_.ResetValues();
        for (const auto& column : select_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return {};
      },
      [&]() -> std::vector<khir::Value> { return {}; });
}

}  // namespace kush::compile