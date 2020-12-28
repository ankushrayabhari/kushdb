#pragma once

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile::cpp {

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

}  // namespace kush::compile::cpp