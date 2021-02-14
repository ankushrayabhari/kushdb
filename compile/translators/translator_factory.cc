#include "compile/translators/translator_factory.h"

#include "compile/ir_registry.h"
#include "compile/translators/cross_product_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/order_by_translator.h"
#include "compile/translators/scan_translator.h"
#include "compile/translators/select_translator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"

namespace kush::compile {

template <typename T>
TranslatorFactory<T>::TranslatorFactory(
    ProgramBuilder<T>& program,
    absl::flat_hash_map<catalog::SqlType,
                        proxy::ForwardDeclaredColumnDataFunctions<T>>&
        functions)
    : program_(program), functions_(functions) {}

template <typename T>
std::vector<std::unique_ptr<OperatorTranslator<T>>>
TranslatorFactory<T>::GetChildTranslators(const plan::Operator& current) {
  std::vector<std::unique_ptr<OperatorTranslator<T>>> translators;
  for (auto& child : current.Children()) {
    translators.push_back(this->Compute(child.get()));
  }
  return translators;
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::ScanOperator& scan) {
  this->Return(std::make_unique<ScanTranslator<T>>(scan, program_, functions_,
                                                   GetChildTranslators(scan)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::SelectOperator& select) {
  this->Return(std::make_unique<SelectTranslator<T>>(
      select, program_, GetChildTranslators(select)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::OutputOperator& output) {
  // this->Return(std::make_unique<OutputTranslator<T>>(
  //    output, program_, GetChildTranslators(output)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::HashJoinOperator& hash_join) {
  // this->Return(std::make_unique<HashJoinTranslator<T>>(
  //    hash_join, program_, GetChildTranslators(hash_join)));
}

template <typename T>
void TranslatorFactory<T>::Visit(
    const plan::GroupByAggregateOperator& group_by_agg) {
  // this->Return(std::make_unique<GroupByAggregateTranslator<T>>(
  //    group_by_agg, program_, GetChildTranslators(group_by_agg)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::OrderByOperator& order_by) {
  this->Return(std::make_unique<OrderByTranslator<T>>(
      order_by, program_, GetChildTranslators(order_by)));
}

template <typename T>
void TranslatorFactory<T>::Visit(
    const plan::CrossProductOperator& cross_product) {
  this->Return(std::make_unique<CrossProductTranslator<T>>(
      cross_product, program_, GetChildTranslators(cross_product)));
}

INSTANTIATE_ON_IR(TranslatorFactory);

}  // namespace kush::compile
