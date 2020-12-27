#pragma once

#include "compile/compilation_context.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(const plan::SelectOperator& select,
                   CompilationContext& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SelectOperator& select_;
  CompilationContext& context_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile