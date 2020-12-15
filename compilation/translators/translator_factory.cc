#include "compilation/translators/translator_factory.h"

#include "compilation/translators/hash_join_translator.h"
#include "compilation/translators/operator_translator.h"
#include "compilation/translators/output_translator.h"
#include "compilation/translators/scan_translator.h"
#include "compilation/translators/select_translator.h"
#include "plan/operator_visitor.h"

namespace kush::compile {

TranslatorFactory::TranslatorFactory(CompliationContext& context)
    : context_(context) {}

std::vector<std::unique_ptr<OperatorTranslator>>
TranslatorFactory::GetChildTranslators(plan::Operator& current) {
  std::vector<std::unique_ptr<OperatorTranslator>> translators;
  for (auto& child : current.Children()) {
    child.get().Accept(*this);
    translators.push_back(GetResult());
  }
  return translators;
}

void TranslatorFactory::Visit(plan::Scan& scan) {
  Return(std::make_unique<ScanTranslator>(scan, context_,
                                          GetChildTranslators(scan)));
}

void TranslatorFactory::Visit(plan::Select& select) {
  Return(std::make_unique<SelectTranslator>(select, context_,
                                            GetChildTranslators(select)));
}

void TranslatorFactory::Visit(plan::Output& output) {
  Return(std::make_unique<OutputTranslator>(output, context_,
                                            GetChildTranslators(output)));
}

void TranslatorFactory::Visit(plan::HashJoin& hash_join) {
  Return(std::make_unique<HashJoinTranslator>(hash_join, context_,
                                              GetChildTranslators(hash_join)));
}

}  // namespace kush::compile
