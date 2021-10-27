#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/operator/order_by_operator.h"

namespace kush::compile {

class OrderByTranslator : public OperatorTranslator {
 public:
  OrderByTranslator(const plan::OrderByOperator& order_by,
                    khir::ProgramBuilder& program,
                    execution::PipelineBuilder& pipeline_builder,
                    std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OrderByTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::OrderByOperator& order_by_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::Vector> buffer_;
  std::unique_ptr<execution::Pipeline> child_pipeline_;
};

}  // namespace kush::compile