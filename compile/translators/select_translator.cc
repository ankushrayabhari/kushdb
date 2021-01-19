#include "compile/translators/select_translator.h"

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/select_operator.h"

namespace kush::compile {

SelectTranslator::SelectTranslator(
    const plan::SelectOperator& select, CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      select_(select),
      context_(context),
      expr_translator_(context, *this) {}

void SelectTranslator::Produce() { Child().Produce(); }

void SelectTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();

  auto& value = expr_translator_.Compute(select_.Expr()).get();

  proxy::If(program, value, [&]() {
    for (const auto& column : select_.Schema().Columns()) {
      auto& value = expr_translator_.Compute(column.Expr()).get();
      values_.AddVariable(value);
    }

    if (auto parent = Parent()) {
      parent->get().Consume(*this);
    }
  });
}

}  // namespace kush::compile