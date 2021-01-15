#pragma once

#include "compile/llvm/llvm_program.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

class OutputTranslator : public OperatorTranslator {
 public:
  OutputTranslator(const plan::OutputOperator& output,
                   CppCompilationContext& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OutputTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  CppCompilationContext& context_;
};

}  // namespace kush::compile