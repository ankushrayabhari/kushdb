#include "compile/translators/output_translator.h"

#include "compile/proxy/printer.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/output_operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    const plan::OutputOperator& output, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(output, std::move(children)), program_(program) {}

void OutputTranslator::Produce() { this->Child().Produce(); }

void OutputTranslator::Consume(OperatorTranslator& src) {
  auto values = this->Child().SchemaValues().Values();
  proxy::Printer printer(program_);
  for (auto& value : values) {
    value.get().Print(printer);
  }
  printer.PrintNewline();
}

}  // namespace kush::compile