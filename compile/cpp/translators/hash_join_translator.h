#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/hash_join_operator.h"

namespace kush::compile::cpp {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(const plan::HashJoinOperator& hash_join,
                     CppCompilationContext& context,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::HashJoinOperator& hash_join_;
  CppCompilationContext& context_;
  ExpressionTranslator expr_translator_;

  std::string hash_table_var_;
  std::string packed_struct_id_;
  std::vector<std::pair<std::string, std::string>> packed_struct_field_ids_;
};

}  // namespace kush::compile::cpp