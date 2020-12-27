#include "compile/cpp_translator.h"

#include "catalog/catalog.h"
#include "compile/compilation_context.h"
#include "compile/program.h"
#include "compile/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

CppTranslator::CppTranslator(const catalog::Database& db,
                             const plan::Operator& op)
    : context_(db), op_(op) {}

void CppTranslator::Translate() {
  context_.Program().CodegenInitialize();

  // Generate code for operator
  TranslatorFactory factory(context_);
  auto translator = factory.Compute(op_);
  translator->Produce();

  context_.Program().CodegenFinalize();
}

Program& CppTranslator::Program() { return context_.Program(); }

}  // namespace kush::compile