#include "compile/translators/output_translator.h"

#include "compile/ir_registry.h"
#include "compile/proxy/printer.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

template <typename T>
OutputTranslator<T>::OutputTranslator(
    const plan::OutputOperator& output, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)), program_(program) {}

template <typename T>
void OutputTranslator<T>::Produce() {
  this->Child().Produce();
}

template <typename T>
void OutputTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto values = this->Child().SchemaValues().Values();

  proxy::Printer printer(program_);
  for (auto& value : values) {
    value.get().Print(printer);
  }
  printer.PrintNewline();
}

INSTANTIATE_ON_IR(OutputTranslator);

}  // namespace kush::compile