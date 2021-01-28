#include "compile/translators/cross_product_translator.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/cross_product_operator.h"

namespace kush::compile {

template <typename T>
CrossProductTranslator<T>::CrossProductTranslator(
    const plan::CrossProductOperator& cross_product, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      cross_product_(cross_product),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void CrossProductTranslator<T>::Produce() {
  this->LeftChild().Produce();
}

template <typename T>
void CrossProductTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto& left_child = this->LeftChild();
  auto& right_child = this->RightChild();

  if (&src == &left_child) {
    right_child.Produce();
  } else {
    for (const auto& column : cross_product_.Schema().Columns()) {
      this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  }
}

INSTANTIATE_ON_IR(CrossProductTranslator);

}  // namespace kush::compile