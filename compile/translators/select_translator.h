#pragma once

#include "compile/llvm/llvm_ir.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

template <typename T>
class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(const plan::SelectOperator& select,
                   ProgramBuilder<T>& program,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SelectOperator& select_;
  ProgramBuilder<T>& program_;
  ExpressionTranslator<T> expr_translator_;
};

}  // namespace kush::compile