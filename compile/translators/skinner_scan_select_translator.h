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

class SkinnerScanSelectTranslator : public OperatorTranslator {
 public:
  SkinnerScanSelectTranslator(
      const plan::SkinnerScanSelectOperator& scan_select,
      khir::ProgramBuilder& program);
  virtual ~SkinnerScanSelectTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::unique_ptr<proxy::DiskMaterializedBuffer> GenerateBuffer();
  bool HasIndex(int col_idx) const;
  std::unique_ptr<proxy::ColumnIndex> GenerateIndex(
      khir::ProgramBuilder& program, int col_idx);

  const plan::SkinnerScanSelectOperator& scan_select_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::vector<int> index_evaluatable_predicates_;
};

}  // namespace kush::compile