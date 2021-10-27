#include "compile/translators/cross_product_translator.h"

#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/cross_product_operator.h"

namespace kush::compile {

CrossProductTranslator::CrossProductTranslator(
    const plan::CrossProductOperator& cross_product,
    khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(cross_product, std::move(children)),
      cross_product_(cross_product),
      expr_translator_(program, *this) {}

void CrossProductTranslator::Produce() { this->Children()[0].get().Produce(); }

void CrossProductTranslator::Consume(OperatorTranslator& src) {
  auto children = this->Children();

  for (int i = 0; i < children.size() - 1; i++) {
    if (&src == &children[i].get()) {
      children[i + 1].get().Produce();
      return;
    }
  }

  this->values_.ResetValues();
  for (const auto& column : cross_product_.Schema().Columns()) {
    this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
  }

  if (auto parent = this->Parent()) {
    parent->get().Consume(*this);
  }
}

}  // namespace kush::compile