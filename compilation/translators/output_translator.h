#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class OutputTranslator : public OperatorTranslator {
 public:
  OutputTranslator(plan::Output& output, CppTranslator& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OutputTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  CppTranslator& context_;
};

}  // namespace kush::compile