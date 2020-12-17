#pragma once

#include "compilation/compilation_context.h"
#include "compilation/translators/expression_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(plan::SelectOperator& select, CompilationContext& context,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  plan::SelectOperator& select_;
  CompilationContext& context_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile