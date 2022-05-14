#pragma once

#include <memory>

#include "compile/proxy/materialized_buffer.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/simd_scan_select_operator.h"

namespace kush::compile {

class SimdScanSelectTranslator : public OperatorTranslator {
 public:
  SimdScanSelectTranslator(const plan::SimdScanSelectOperator& scan_select,
                           khir::ProgramBuilder& program);
  virtual ~SimdScanSelectTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::unique_ptr<proxy::DiskMaterializedBuffer> GenerateBuffer();
  const plan::SimdScanSelectOperator& scan_select_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile