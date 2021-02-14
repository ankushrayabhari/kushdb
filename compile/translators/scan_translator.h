#pragma once

#include "absl/container/flat_hash_map.h"
#include "compile/program_builder.h"
#include "compile/proxy/column_data.h"
#include "compile/translators/operator_translator.h"
#include "plan/scan_operator.h"

namespace kush::compile {

template <typename T>
class ScanTranslator : public OperatorTranslator<T> {
 public:
  ScanTranslator(
      const plan::ScanOperator& scan, ProgramBuilder<T>& program,
      absl::flat_hash_map<catalog::SqlType,
                          proxy::ForwardDeclaredColumnDataFunctions<T>>&
          functions,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~ScanTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::ScanOperator& scan_;
  ProgramBuilder<T>& program_;
  absl::flat_hash_map<catalog::SqlType,
                      proxy::ForwardDeclaredColumnDataFunctions<T>>& functions_;
};

}  // namespace kush::compile