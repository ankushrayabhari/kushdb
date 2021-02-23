#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/hash_table.h"
#include "compile/proxy/ptr.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile {

template <typename T>
class GroupByAggregateTranslator : public OperatorTranslator<T> {
 public:
  GroupByAggregateTranslator(
      const plan::GroupByAggregateOperator& group_by_agg,
      proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
      proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs,
      ProgramBuilder<T>& program,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~GroupByAggregateTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::GroupByAggregateOperator& group_by_agg_;
  ProgramBuilder<T>& program_;
  proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs_;
  proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs_;
  ExpressionTranslator<T> expr_translator_;
  std::unique_ptr<proxy::HashTable<T>> buffer_;
  std::unique_ptr<proxy::Ptr<T, proxy::Bool<T>>> found_;
};

}  // namespace kush::compile