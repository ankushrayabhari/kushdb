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

void CrossProductTranslator::Produce() { this->LeftChild().Produce(); }

void CrossProductTranslator::Consume(OperatorTranslator& src) {
  auto& left_child = this->LeftChild();
  auto& right_child = this->RightChild();

  if (&src == &left_child) {
    right_child.Produce();
  } else {
    this->values_.ResetValues();
    for (const auto& column : cross_product_.Schema().Columns()) {
      this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  }
}

}  // namespace kush::compile