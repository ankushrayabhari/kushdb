#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(const plan::HashJoinOperator& hash_join,
                     khir::ProgramBuilder& program,
                     execution::PipelineBuilder& pipeline_builder,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::HashJoinOperator& hash_join_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::HashTable> buffer_;
  std::unique_ptr<execution::Pipeline> left_pipeline_;
};

}  // namespace kush::compile