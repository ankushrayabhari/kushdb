#include "compilation/cpp_translator.h"

#include "catalog/catalog.h"
#include "compilation/compilation_context.h"
#include "compilation/program.h"
#include "compilation/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

CppTranslator::CppTranslator(const catalog::Database& db, plan::Operator& op)
    : context_(db), op_(op) {}

void CppTranslator::Translate() {
  context_.Program().CodegenInitialize();

  // Generate code for operator
  TranslatorFactory factory(context_);
  auto translator = factory.Produce(op_);
  translator->Produce();

  context_.Program().CodegenFinalize();
}

Program& CppTranslator::Program() { return context_.Program(); }

}  // namespace kush::compile