#include "compilation/translators/translator_factory.h"

#include "compilation/translators/scan_translator.h"
#include "plan/operator_visitor.h"

namespace kush::compile {

TranslatorFactory::TranslatorFactory(CppTranslator& context)
    : context_(context) {}

void TranslatorFactory::Return(std::unique_ptr<Translator> result) {
  result_ = std::move(result);
}

std::unique_ptr<Translator> TranslatorFactory::GetResult() {
  auto result = std::move(result_);
  result_ = nullptr;
  return result;
}

std::vector<std::unique_ptr<Translator>> TranslatorFactory::GetChildTranslators(
    plan::Operator& current) {
  std::vector<std::unique_ptr<Translator>> translators;
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
  Return(std::make_unique<ScanTranslator>(select, context_,
                                          GetChildTranslators(select)));
}

void TranslatorFactory::Visit(plan::Output& output) {
  Return(std::make_unique<ScanTranslator>(output, context_,
                                          GetChildTranslators(output)));
}

void TranslatorFactory::Visit(plan::HashJoin& hash_join) {
  Return(std::make_unique<ScanTranslator>(hash_join, context_,
                                          GetChildTranslators(hash_join)));
}

}  // namespace kush::compile
