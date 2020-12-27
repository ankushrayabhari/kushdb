#include "compile/translators/translator_factory.h"

#include "compile/translators/group_by_aggregate_translator.h"
#include "compile/translators/hash_join_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/output_translator.h"
#include "compile/translators/scan_translator.h"
#include "compile/translators/select_translator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"

namespace kush::compile {

TranslatorFactory::TranslatorFactory(CompilationContext& context)
    : context_(context) {}

std::vector<std::unique_ptr<OperatorTranslator>>
TranslatorFactory::GetChildTranslators(const plan::Operator& current) {
  std::vector<std::unique_ptr<OperatorTranslator>> translators;
  for (auto& child : current.Children()) {
    translators.push_back(Compute(child.get()));
  }
  return translators;
}

void TranslatorFactory::Visit(const plan::ScanOperator& scan) {
  Return(std::make_unique<ScanTranslator>(scan, context_,
                                          GetChildTranslators(scan)));
}

void TranslatorFactory::Visit(const plan::SelectOperator& select) {
  Return(std::make_unique<SelectTranslator>(select, context_,
                                            GetChildTranslators(select)));
}

void TranslatorFactory::Visit(const plan::OutputOperator& output) {
  Return(std::make_unique<OutputTranslator>(output, context_,
                                            GetChildTranslators(output)));
}

void TranslatorFactory::Visit(const plan::HashJoinOperator& hash_join) {
  Return(std::make_unique<HashJoinTranslator>(hash_join, context_,
                                              GetChildTranslators(hash_join)));
}

void TranslatorFactory::Visit(
    const plan::GroupByAggregateOperator& group_by_agg) {
  Return(std::make_unique<GroupByAggregateTranslator>(
      group_by_agg, context_, GetChildTranslators(group_by_agg)));
}

}  // namespace kush::compile
