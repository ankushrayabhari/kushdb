#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(plan::Select& scan, CppTranslator& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  plan::Select& select_;
  CppTranslator& context_;
};

}  // namespace kush::compile