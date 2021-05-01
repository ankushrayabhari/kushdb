#include "compile/translators/select_translator.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/select_operator.h"

namespace kush::compile {

template <typename T>
SelectTranslator<T>::SelectTranslator(
    const plan::SelectOperator& select, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(select, std::move(children)),
      select_(select),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void SelectTranslator<T>::Produce() {
  this->Child().Produce();
}

template <typename T>
void SelectTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto value =
      expr_translator_.template ComputeAs<proxy::Bool<T>>(select_.Expr());

  proxy::If(program_, value, [&]() {
    this->values_.ResetValues();
    for (const auto& column : select_.Schema().Columns()) {
      this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
    }

    if (auto parent = this->Parent()) {
      parent->get().Consume(*this);
    }
  });
}

INSTANTIATE_ON_IR(SelectTranslator);

}  // namespace kush::compile