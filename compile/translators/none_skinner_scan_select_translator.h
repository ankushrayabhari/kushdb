#pragma once

#include <memory>

#include "compile/proxy/column_data.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_scan_select_operator.h"

namespace kush::compile {

class NoneSkinnerScanSelectTranslator : public OperatorTranslator {
 public:
  NoneSkinnerScanSelectTranslator(
      const plan::SkinnerScanSelectOperator& scan_select,
      khir::ProgramBuilder& program);
  virtual ~NoneSkinnerScanSelectTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::unique_ptr<proxy::DiskMaterializedBuffer> GenerateBuffer();

  const plan::SkinnerScanSelectOperator& scan_select_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::vector<int> index_evaluatable_predicates_;
};

}  // namespace kush::compile