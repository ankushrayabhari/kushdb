#include "catalog/catalog.h"
#include "compile/llvm/llvm_program.h"
#include "compile/program.h"
#include "compile/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

CppTranslator::CppTranslator(const catalog::Database& db,
                             const plan::Operator& op)
    : db_(db), op_(op) {}

Program CppTranslator::Translate() {
  context_.Program().CodegenInitialize();

  // Generate code for operator
  TranslatorFactory factory(context_);
  auto translator = factory.Compute(op_);
  translator->Produce();

  context_.Program().CodegenFinalize();
}

}  // namespace kush::compile