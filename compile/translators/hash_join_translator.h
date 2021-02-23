#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

template <typename T>
class HashJoinTranslator : public OperatorTranslator<T> {
 public:
  HashJoinTranslator(
      const plan::HashJoinOperator& hash_join, ProgramBuilder<T>& program,
      proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
      proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::HashJoinOperator& hash_join_;
  ProgramBuilder<T>& program_;
  proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs_;
  proxy::ForwardDeclaredHashTableFunctions<T>& hash_funcs_;
  ExpressionTranslator<T> expr_translator_;
  std::unique_ptr<proxy::HashTable<T>> buffer_;
};

}  // namespace kush::compile