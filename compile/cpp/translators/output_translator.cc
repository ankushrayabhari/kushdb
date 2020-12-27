#include "compile/cpp/translators/output_translator.h"

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    const plan::OutputOperator& output, CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)), context_(context) {}

void OutputTranslator::Produce() { Child().Produce(); }

void OutputTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  const auto& values = Child().GetValues().Values();
  if (values.empty()) {
    return;
  }

  bool first = true;
  for (const auto& [variable, type] : values) {
    if (type == "double") {
      program.fout << "std::cout << std::fixed;";
      program.fout << "std::cout << std::setprecision(2);";
    }
    program.fout << "std::cout";

    if (first) {
      program.fout << " << " << variable << ";";
      first = false;
    } else {
      program.fout << " << \",\" << " << variable << ";";
    }
  }

  program.fout << "std::cout << \"\\n\";\n";
}

}  // namespace kush::compile