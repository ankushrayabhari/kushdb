#include "compile/translators/select_translator.h"

#include "compile/llvm/llvm_program.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/types.h"
#include "plan/select_operator.h"

namespace kush::compile {

SelectTranslator::SelectTranslator(
    const plan::SelectOperator& select, CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      select_(select),
      context_(context),
      expr_translator_(context, *this) {}

void SelectTranslator::Produce() { Child().Produce(); }

void SelectTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();

  program.fout << "if (\n"
               << expr_translator_.Compute(select_.Expr())->Get() << ") {\n";

  for (const auto& column : select_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());

    program.fout << "auto " << var << " = "
                 << expr_translator_.Compute(column.Expr())->Get();
    program.fout << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}\n";
}

}  // namespace kush::compile