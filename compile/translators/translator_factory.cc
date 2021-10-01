#include "compile/translators/translator_factory.h"

#include <string>

#include "absl/flags/flag.h"

#include "compile/translators/cross_product_translator.h"
#include "compile/translators/group_by_aggregate_translator.h"
#include "compile/translators/hash_join_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/order_by_translator.h"
#include "compile/translators/output_translator.h"
#include "compile/translators/permutable_skinner_join_translator.h"
#include "compile/translators/recompiling_skinner_join_translator.h"
#include "compile/translators/scan_translator.h"
#include "compile/translators/select_translator.h"
#include "khir/program_builder.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "plan/skinner_join_operator.h"

ABSL_FLAG(std::string, skinner, "permute",
          "Skinner Join Implementation: permute or recompile");

namespace kush::compile {

TranslatorFactory::TranslatorFactory(
    khir::ProgramBuilder& program, execution::PipelineBuilder& pipeline_builder)
    : program_(program), pipeline_builder_(pipeline_builder) {}

std::vector<std::unique_ptr<OperatorTranslator>>
TranslatorFactory::GetChildTranslators(const plan::Operator& current) {
  std::vector<std::unique_ptr<OperatorTranslator>> translators;
  for (auto& child : current.Children()) {
    translators.push_back(this->Compute(child.get()));
  }
  return translators;
}

void TranslatorFactory::Visit(const plan::ScanOperator& scan) {
  this->Return(std::make_unique<ScanTranslator>(
      scan, program_, pipeline_builder_, GetChildTranslators(scan)));
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
  if (FLAGS_skinner.CurrentValue() == "permute") {
    this->Return(std::make_unique<PermutableSkinnerJoinTranslator>(
        skinner_join, program_, GetChildTranslators(skinner_join)));
    return;
  }

  if (FLAGS_skinner.CurrentValue() == "recompile") {
    this->Return(std::make_unique<RecompilingSkinnerJoinTranslator>(
        skinner_join, program_, pipeline_builder_,
        GetChildTranslators(skinner_join)));
    return;
  }

  throw std::runtime_error("Unknown skinner join implementation.");
}

void TranslatorFactory::Visit(const plan::HashJoinOperator& hash_join) {
  this->Return(std::make_unique<HashJoinTranslator>(
      hash_join, program_, pipeline_builder_, GetChildTranslators(hash_join)));
}

void TranslatorFactory::Visit(
    const plan::GroupByAggregateOperator& group_by_agg) {
  this->Return(std::make_unique<GroupByAggregateTranslator>(
      group_by_agg, program_, pipeline_builder_,
      GetChildTranslators(group_by_agg)));
}

void TranslatorFactory::Visit(const plan::OrderByOperator& order_by) {
  this->Return(std::make_unique<OrderByTranslator>(
      order_by, program_, pipeline_builder_, GetChildTranslators(order_by)));
}

void TranslatorFactory::Visit(const plan::CrossProductOperator& cross_product) {
  this->Return(std::make_unique<CrossProductTranslator>(
      cross_product, program_, GetChildTranslators(cross_product)));
}

}  // namespace kush::compile
