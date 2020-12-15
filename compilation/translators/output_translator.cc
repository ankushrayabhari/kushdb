#include "compilation/translators/output_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

OutputTranslator::OutputTranslator(
    plan::Output& output, CppTranslator& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      output_(output),
      context_(context) {}

void OutputTranslator::Produce() {}

void OutputTranslator::Consume(OperatorTranslator& src) {}

}  // namespace kush::compile