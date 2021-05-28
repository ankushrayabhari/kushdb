#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/ptr.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile {

class GroupByAggregateTranslator : public OperatorTranslator {
 public:
  GroupByAggregateTranslator(
      const plan::GroupByAggregateOperator& group_by_agg,
      khir::KHIRProgramBuilder& program,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~GroupByAggregateTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  proxy::Float64 ToFloat(proxy::Value& v);

  const plan::GroupByAggregateOperator& group_by_agg_;
  khir::KHIRProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::HashTable> buffer_;
  std::unique_ptr<proxy::Ptr<proxy::Bool>> found_;
};

}  // namespace kush::compile