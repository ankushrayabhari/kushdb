#pragma once

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

class OutputTranslator : public OperatorTranslator {
 public:
  OutputTranslator(plan::OutputOperator& output, CompilationContext& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OutputTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  CompilationContext& context_;
};

}  // namespace kush::compile