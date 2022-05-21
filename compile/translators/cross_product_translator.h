#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/cross_product_operator.h"

namespace kush::compile {

class CrossProductTranslator : public OperatorTranslator {
 public:
  CrossProductTranslator(
      const plan::CrossProductOperator& cross_product,
      khir::ProgramBuilder& program,
      execution::PipelineBuilder& pipeline_builder,
      execution::QueryState& state,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~CrossProductTranslator() = default;

  void Produce(proxy::Pipeline& output) override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::CrossProductOperator& cross_product_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::Vector> buffer_;
};

}  // namespace kush::compile