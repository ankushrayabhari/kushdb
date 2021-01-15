#include "compile/query_translator.h"

#include <memory>

#include "catalog/catalog.h"
#include "compile/llvm/llvm_program.h"
#include "compile/program.h"
#include "compile/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const catalog::Database& db,
                                 const plan::Operator& op)
    : db_(db), op_(op) {}

std::unique_ptr<Program> QueryTranslator::Translate() {
  auto program = std::make_unique<LLVMProgram>();

  // Generate code for operator
  TranslatorFactory factory(*program);
  auto translator = factory.Compute(op_);
  translator->Produce();

  return std::move(program);
}

}  // namespace kush::compile