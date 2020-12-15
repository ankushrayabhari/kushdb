#include "compilation/translators/select_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

SelectTranslator::SelectTranslator(
    plan::Select& select, CppTranslator& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      select_(select),
      context_(context) {}

void SelectTranslator::Produce() {}

void SelectTranslator::Consume(OperatorTranslator& src) {}

}  // namespace kush::compile