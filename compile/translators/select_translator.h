#pragma once

#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/operator.h"

namespace kush::compile {

class SelectTranslator : public OperatorTranslator {
 public:
  SelectTranslator(const plan::SelectOperator& select,
                   khir::ProgramBuilder& program,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::SelectOperator& select_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile