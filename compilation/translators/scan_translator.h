#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(plan::Scan& scan, CppTranslator& context,
                 std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~ScanTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  plan::Scan& scan_;
  CppTranslator& context_;
};

}  // namespace kush::compile