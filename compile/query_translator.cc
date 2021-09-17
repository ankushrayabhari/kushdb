#include "compile/query_translator.h"

#include <memory>

#include "compile/backend.h"
#include "compile/forward_declare.h"
#include "compile/translators/translator_factory.h"
#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "plan/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

std::pair<std::unique_ptr<OperatorTranslator>, std::unique_ptr<Program>>
QueryTranslator::Translate() {
  khir::ProgramBuilder program;

  ForwardDeclare(program);

  // Create the compute function
  program.CreatePublicFunction(program.VoidType(), {}, "compute");

  // Generate code for operator
  TranslatorFactory factory(program);
  auto translator = factory.Compute(op_);
  translator->Produce();

  // terminate last basic block
  program.Return();

  // khir::ProgramPrinter printer;
  // program.Translate(printer);

  switch (GetBackend()) {
    case Backend::ASM: {
      auto backend =
          std::make_unique<khir::ASMBackend>(khir::GetRegAllocImpl());
      program.Translate(*backend);
      return {std::move(translator), std::move(backend)};
    }

    case Backend::LLVM: {
      auto backend = std::make_unique<khir::LLVMBackend>();
      program.Translate(*backend);
      return {std::move(translator), std::move(backend)};
    }
  }
}

}  // namespace kush::compile