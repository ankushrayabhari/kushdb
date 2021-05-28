#include "compile/translators/translator_factory.h"

#include "compile/khir/khir_program_builder.h"
#include "compile/translators/cross_product_translator.h"
#include "compile/translators/group_by_aggregate_translator.h"
#include "compile/translators/hash_join_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/order_by_translator.h"
#include "compile/translators/output_translator.h"
#include "compile/translators/scan_translator.h"
#include "compile/translators/select_translator.h"
#include "compile/translators/skinner_join_translator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "plan/skinner_join_operator.h"

namespace kush::compile {

TranslatorFactory::TranslatorFactory(khir::KHIRProgramBuilder& program)
    : program_(program) {}

std::vector<std::unique_ptr<OperatorTranslator>>
TranslatorFactory::GetChildTranslators(const plan::Operator& current) {
  std::vector<std::unique_ptr<OperatorTranslator>> translators;
  for (auto& child : current.Children()) {
    translators.push_back(this->Compute(child.get()));
  }
  return translators;
}

void TranslatorFactory::Visit(const plan::ScanOperator& scan) {
  this->Return(std::make_unique<ScanTranslator>(scan, program_,
                                                GetChildTranslators(scan)));
}

void TranslatorFactory::Visit(const plan::SelectOperator& select) {
  this->Return(std::make_unique<SelectTranslator>(select, program_,
                                                  GetChildTranslators(select)));
}

void TranslatorFactory::Visit(const plan::OutputOperator& output) {
  this->Return(std::make_unique<OutputTranslator>(output, program_,
                                                  GetChildTranslators(output)));
}

void TranslatorFactory::Visit(const plan::SkinnerJoinOperator& skinner_join) {
  this->Return(std::make_unique<SkinnerJoinTranslator>(
      skinner_join, program_, GetChildTranslators(skinner_join)));
}

void TranslatorFactory::Visit(const plan::HashJoinOperator& hash_join) {
  this->Return(std::make_unique<HashJoinTranslator>(
      hash_join, program_, GetChildTranslators(hash_join)));
}

void TranslatorFactory::Visit(
    const plan::GroupByAggregateOperator& group_by_agg) {
  this->Return(std::make_unique<GroupByAggregateTranslator>(
      group_by_agg, program_, GetChildTranslators(group_by_agg)));
}

void TranslatorFactory::Visit(const plan::OrderByOperator& order_by) {
  this->Return(std::make_unique<OrderByTranslator>(
      order_by, program_, GetChildTranslators(order_by)));
}

void TranslatorFactory::Visit(const plan::CrossProductOperator& cross_product) {
  this->Return(std::make_unique<CrossProductTranslator>(
      cross_product, program_, GetChildTranslators(cross_product)));
}

}  // namespace kush::compile
