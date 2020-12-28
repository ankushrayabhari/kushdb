#pragma once

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/scan_operator.h"

namespace kush::compile::cpp {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(const plan::ScanOperator& scan, CppCompilationContext& context,
                 std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~ScanTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::ScanOperator& scan_;
  CppCompilationContext& context_;
};

}  // namespace kush::compile::cpp