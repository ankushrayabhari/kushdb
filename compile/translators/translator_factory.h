#pragma once

#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "compile/llvm/llvm_ir.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/vector.h"
#include "compile/translators/operator_translator.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

template <typename T>
class TranslatorFactory
    : public util::Visitor<plan::ImmutableOperatorVisitor,
                           const plan::Operator&, OperatorTranslator<T>> {
 public:
  TranslatorFactory(
      ProgramBuilder<T>& program,
      absl::flat_hash_map<catalog::SqlType,
                          proxy::ForwardDeclaredColumnDataFunctions<T>>&
          col_data_funcs,
      proxy::ForwardDeclaredPrintFunctions<T>& print_funcs,
      proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs);
  virtual ~TranslatorFactory() = default;

  void Visit(const plan::ScanOperator& scan) override;
  void Visit(const plan::SelectOperator& select) override;
  void Visit(const plan::OutputOperator& output) override;
  void Visit(const plan::HashJoinOperator& hash_join) override;
  void Visit(const plan::CrossProductOperator& cross_product) override;
  void Visit(const plan::GroupByAggregateOperator& group_by_agg) override;
  void Visit(const plan::OrderByOperator& order_by) override;

 private:
  std::vector<std::unique_ptr<OperatorTranslator<T>>> GetChildTranslators(
      const plan::Operator& current);
  ProgramBuilder<T>& program_;
  absl::flat_hash_map<catalog::SqlType,
                      proxy::ForwardDeclaredColumnDataFunctions<T>>&
      col_data_funcs_;
  proxy::ForwardDeclaredPrintFunctions<T>& print_funcs_;
  proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs_;
};

}  // namespace kush::compile
