#include "compile/query_translator.h"

#include <memory>

#include "compile/forward_declare.h"
#include "compile/proxy/pipeline.h"
#include "compile/translators/translator_factory.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
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
  execution::QueryState state;

  ForwardDeclare(program_builder);

  // Generate code for operator
  TranslatorFactory factory(program_builder, pipeline_builder, state);
  auto translator = factory.Compute(op_);

  proxy::Pipeline output_pipeline(program_builder, pipeline_builder);
  translator->Produce(output_pipeline);
  output_pipeline.Build();

  auto program = program_builder.Build();

  // khir::ProgramPrinter printer;
  // printer.Translate(program);

  std::unique_ptr<khir::Backend> backend;
  switch (khir::GetBackendType()) {
    case khir::BackendType::ASM: {
      auto b =
          std::make_unique<khir::ASMBackend>(program, khir::GetRegAllocImpl());
      b->Compile();
      backend = std::move(b);
      break;
    }

    case khir::BackendType::LLVM: {
      auto b = std::make_unique<khir::LLVMBackend>(program);
      b->Translate();
      b->Compile();
      backend = std::move(b);
      break;
    }
  }
  return execution::ExecutableQuery(std::move(translator), std::move(backend),
                                    std::move(pipeline_builder),
                                    std::move(state));
}

}  // namespace kush::compile