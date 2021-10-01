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
#include "plan/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

execution::ExecutableQuery QueryTranslator::Translate() {
  khir::ProgramBuilder program;
  execution::PipelineBuilder pipeline_builder;

  ForwardDeclare(program);

  // Generate code for operator
  TranslatorFactory factory(program, pipeline_builder);
  auto translator = factory.Compute(op_);
  translator->Produce();

  auto output_pipeline = pipeline_builder.FinishPipeline();

  // khir::ProgramPrinter printer;
  // program.Translate(printer);

  switch (khir::GetBackendType()) {
    case khir::BackendType::ASM: {
      auto backend =
          std::make_unique<khir::ASMBackend>(khir::GetRegAllocImpl());
      program.Translate(*backend);
      return execution::ExecutableQuery(std::move(translator),
                                        std::move(backend),
                                        std::move(output_pipeline));
    }

    case khir::BackendType::LLVM: {
      auto backend = std::make_unique<khir::LLVMBackend>();
      program.Translate(*backend);
      return execution::ExecutableQuery(std::move(translator),
                                        std::move(backend),
                                        std::move(output_pipeline));
    }
  }
}

}  // namespace kush::compile