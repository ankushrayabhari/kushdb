#pragma once

#include "absl/container/flat_hash_map.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/translators/operator_translator.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/scan_operator.h"

namespace kush::compile {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(const plan::ScanOperator& scan, khir::ProgramBuilder& program,
                 execution::QueryState& state);
  virtual ~ScanTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;
  std::unique_ptr<proxy::DiskMaterializedBuffer> GenerateBuffer();
  bool HasIndex(int col_idx);
  std::unique_ptr<proxy::ColumnIndex> GenerateIndex(int col_idx);

 private:
  const plan::ScanOperator& scan_;
  khir::ProgramBuilder& program_;
  execution::QueryState& state_;
};

}  // namespace kush::compile