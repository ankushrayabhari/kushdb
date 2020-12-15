#include "compilation/translators/output_translator.h"

#include "compilation/compilation_context.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    plan::Output& output, CompliationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)), context_(context) {}

void OutputTranslator::Produce() { Child().Produce(); }

void OutputTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  const auto& values = Child().GetValues().Values();
  if (values.empty()) {
    return;
  }

  program.fout << "std::cout";

  bool first = true;
  for (const auto& [variable, type] : values) {
    if (first) {
      program.fout << " << " << variable;
      first = false;
    } else {
      program.fout << " << \",\" << " << variable;
    }
  }

  program.fout << " << \"\\n\";\n";
}

}  // namespace kush::compile