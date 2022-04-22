#include "compile/query_translator.h"

#include <memory>

#include "compile/forward_declare.h"
#include "compile/translators/translator_factory.h"
#include "execution/pipeline.h"
#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "plan/operator/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

execution::ExecutableQuery QueryTranslator::Translate() {
  khir::ProgramBuilder program_builder;
  execution::PipelineBuilder pipeline_builder;

  ForwardDeclare(program_builder);

  // Generate code for operator
  TranslatorFactory factory(program_builder, pipeline_builder);
  auto translator = factory.Compute(op_);
  translator->Produce();

  auto output_pipeline = pipeline_builder.FinishPipeline();
  auto program = program_builder.Build();

  // khir::ProgramPrinter printer;
  // printer.Translate(program);

  std::unique_ptr<khir::Backend> backend;
  switch (khir::GetBackendType()) {
    case khir::BackendType::ASM: {
      backend = std::make_unique<khir::ASMBackend>(khir::GetRegAllocImpl());
      break;
    }

    case khir::BackendType::LLVM: {
      backend = std::make_unique<khir::LLVMBackend>();
      break;
    }
  }
  backend->Translate(program);
  return execution::ExecutableQuery(std::move(translator), std::move(backend),
                                    std::move(output_pipeline));
}

}  // namespace kush::compile