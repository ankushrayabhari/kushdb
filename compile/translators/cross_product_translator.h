#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/llvm/llvm_ir.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/cross_product_operator.h"

namespace kush::compile {

template <typename T>
class CrossProductTranslator : public OperatorTranslator<T> {
 public:
  CrossProductTranslator(
      const plan::CrossProductOperator& cross_product,
      ProgramBuilder<T>& program,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~CrossProductTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::CrossProductOperator& cross_product_;
  ProgramBuilder<T>& program_;
  ExpressionTranslator<T> expr_translator_;
};

}  // namespace kush::compile