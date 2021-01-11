#include "compile/cpp/translators/cross_product_translator.h"

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "compile/cpp/types.h"
#include "plan/cross_product_operator.h"

namespace kush::compile::cpp {

CrossProductTranslator::CrossProductTranslator(
    const plan::CrossProductOperator& cross_product,
    CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      cross_product_(cross_product),
      context_(context),
      expr_translator_(context, *this) {}

void CrossProductTranslator::Produce() { LeftChild().Produce(); }

void CrossProductTranslator::Consume(OperatorTranslator& src) {
  auto& left_child = LeftChild();
  auto& right_child = RightChild();
  auto& program = context_.Program();

  if (&src == &left_child) {
    right_child.Produce();
  } else {
    for (const auto& column : cross_product_.Schema().Columns()) {
      auto var = program.GenerateVariable();
      auto type = SqlTypeToRuntimeType(column.Expr().Type());

      program.fout << "auto " << var << " = "
                   << expr_translator_.Compute(column.Expr())->Get() << ";\n";

      values_.AddVariable(var, type);
    }

    if (auto parent = Parent()) {
      parent->get().Consume(*this);
    }
  }
}

}  // namespace kush::compile::cpp