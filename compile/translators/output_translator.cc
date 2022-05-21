#include "compile/translators/output_translator.h"

#include <iostream>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/output_operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    const plan::OutputOperator& output, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(output, std::move(children)),
      program_(program),
      empty_(proxy::String::Global(program_, "")) {}

void OutputTranslator::Produce(proxy::Pipeline& output) {
  this->Child().Produce(output);
}

void OutputTranslator::Consume(OperatorTranslator& src) {
  proxy::Printer printer(program_);
  for (const auto& value : this->Child().SchemaValues().Values()) {
    proxy::If(
        program_, value.IsNull(), [&]() { printer.Print(empty_); },
        [&]() { value.Get().Print(printer); });
  }
  printer.PrintNewline();
}

}  // namespace kush::compile