#include "compile/translators/output_translator.h"

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/output_operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    const plan::OutputOperator& output, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(output, std::move(children)),
      program_(program),
      pipeline_builder_(pipeline_builder),
      empty_(proxy::String::Global(program_, "")) {}

void OutputTranslator::Produce() {
  auto& pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
  this->Child().Produce();
  program_.Return();
}

void OutputTranslator::Consume(OperatorTranslator& src) {
  auto values = this->Child().SchemaValues().Values();
  proxy::Printer printer(program_);
  for (auto& value_ref : values) {
    const auto& value = value_ref.get();
    proxy::If(
        program_, value.IsNull(), [&]() { printer.Print(empty_); },
        [&]() { value.Get().Print(printer); });
  }
  printer.PrintNewline();
}

}  // namespace kush::compile