#pragma once

#include "compile/llvm/llvm_program.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(const plan::SelectOperator& select,
                   CppCompilationContext& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SelectOperator& select_;
  CppCompilationContext& context_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile