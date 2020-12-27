#pragma once

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/scan_operator.h"

namespace kush::compile {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(const plan::ScanOperator& scan, CompilationContext& context,
                 std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~ScanTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::ScanOperator& scan_;
  CompilationContext& context_;
};

}  // namespace kush::compile