#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/cross_product_operator.h"

namespace kush::compile {

class CrossProductTranslator : public OperatorTranslator {
 public:
  CrossProductTranslator(
      const plan::CrossProductOperator& cross_product,
      khir::ProgramBuilder& program,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~CrossProductTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::CrossProductOperator& cross_product_;
  ExpressionTranslator expr_translator_;
};

}  // namespace kush::compile