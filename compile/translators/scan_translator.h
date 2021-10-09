#pragma once

#include "absl/container/flat_hash_map.h"

#include "compile/proxy/column_data.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/scan_operator.h"

namespace kush::compile {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(const plan::ScanOperator& scan, khir::ProgramBuilder& program,
                 std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~ScanTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::ScanOperator& scan_;
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile