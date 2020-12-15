#include "compilation/translators/select_translator.h"

#include "compilation/compilation_context.h"
#include "compilation/translators/expression_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

SelectTranslator::SelectTranslator(
    plan::Select& select, CompliationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      select_(select),
      context_(context) {}

void SelectTranslator::Produce() { Child().Produce(); }

void SelectTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();

  program.fout << "if (\n";
  ExpressionTranslator translator(context_, Child());
  select_.expression->Accept(translator);
  program.fout << ") {\n";

  SetSchemaValues(Child().GetValues());

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}\n";
}

}  // namespace kush::compile