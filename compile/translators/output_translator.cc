#include "compile/translators/output_translator.h"

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    plan::OutputOperator& output, CompilationContext& context,
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