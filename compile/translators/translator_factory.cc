#include "compile/translators/translator_factory.h"

#include "compile/ir_registry.h"
#include "compile/translators/cross_product_translator.h"
#include "compile/translators/group_by_aggregate_translator.h"
#include "compile/translators/hash_join_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/order_by_translator.h"
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

template <typename T>
TranslatorFactory<T>::TranslatorFactory(
    ProgramBuilder<T>& program,
    absl::flat_hash_map<catalog::SqlType,
                        proxy::ForwardDeclaredColumnDataFunctions<T>>&
        col_data_funcs,
    proxy::ForwardDeclaredPrintFunctions<T>& print_funcs,
    proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
    proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs)
    : program_(program),
      col_data_funcs_(col_data_funcs),
      print_funcs_(print_funcs),
      vector_funcs_(vector_funcs),
      hash_funcs_(hash_funcs) {}

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
  this->Return(std::make_unique<ScanTranslator<T>>(
      scan, program_, col_data_funcs_, GetChildTranslators(scan)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::SelectOperator& select) {
  this->Return(std::make_unique<SelectTranslator<T>>(
      select, program_, GetChildTranslators(select)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::OutputOperator& output) {
  this->Return(std::make_unique<OutputTranslator<T>>(
      output, program_, print_funcs_, GetChildTranslators(output)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::HashJoinOperator& hash_join) {
  this->Return(std::make_unique<HashJoinTranslator<T>>(
      hash_join, program_, vector_funcs_, hash_funcs_,
      GetChildTranslators(hash_join)));
}

template <typename T>
void TranslatorFactory<T>::Visit(
    const plan::GroupByAggregateOperator& group_by_agg) {
  this->Return(std::make_unique<GroupByAggregateTranslator<T>>(
      group_by_agg, vector_funcs_, hash_funcs_, program_,
      GetChildTranslators(group_by_agg)));
}

template <typename T>
void TranslatorFactory<T>::Visit(const plan::OrderByOperator& order_by) {
  this->Return(std::make_unique<OrderByTranslator<T>>(
      order_by, program_, vector_funcs_, GetChildTranslators(order_by)));
}

template <typename T>
void TranslatorFactory<T>::Visit(
    const plan::CrossProductOperator& cross_product) {
  this->Return(std::make_unique<CrossProductTranslator<T>>(
      cross_product, program_, GetChildTranslators(cross_product)));
}

INSTANTIATE_ON_IR(TranslatorFactory);

}  // namespace kush::compile
