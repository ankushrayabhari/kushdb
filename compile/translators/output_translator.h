#pragma once

#include "compile/proxy/value/ir_value.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"
#include "plan/output_operator.h"

namespace kush::compile {

class OutputTranslator : public OperatorTranslator {
 public:
  OutputTranslator(const plan::OutputOperator& output,
                   khir::ProgramBuilder& program,
                   execution::PipelineBuilder& pipeline_builder,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OutputTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
};

}  // namespace kush::compile