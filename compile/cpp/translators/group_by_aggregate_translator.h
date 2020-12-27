#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile {

class GroupByAggregateTranslator : public OperatorTranslator {
 public:
  GroupByAggregateTranslator(
      const plan::GroupByAggregateOperator& group_by_agg,
      CppCompilationContext& context,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~GroupByAggregateTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::GroupByAggregateOperator& group_by_agg_;
  CppCompilationContext& context_;
  ExpressionTranslator expr_translator_;
  std::string hash_table_var_;
  std::string packed_struct_id_;
  std::vector<std::pair<std::string, std::string>> packed_group_by_field_type_;
  std::vector<std::pair<std::string, std::string>> packed_agg_field_type_;
  std::string record_count_field_;
};

}  // namespace kush::compile