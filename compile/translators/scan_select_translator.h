#pragma once

#include <memory>

#include "compile/proxy/column_data.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/scan_select_operator.h"

namespace kush::compile {

class ScanSelectTranslator : public OperatorTranslator {
 public:
  ScanSelectTranslator(const plan::ScanSelectOperator& scan_select,
                       khir::ProgramBuilder& program,
                       execution::PipelineBuilder& pipeline_builder,
                       execution::QueryState& state);
  virtual ~ScanSelectTranslator() = default;

  void Produce(proxy::Pipeline& output) override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::unique_ptr<proxy::DiskMaterializedBuffer> GenerateBuffer();

  const plan::ScanSelectOperator& scan_select_;
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile