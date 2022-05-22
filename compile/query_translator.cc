#include "compile/query_translator.h"

#include <memory>

#include "compile/forward_declare.h"
#include "compile/proxy/pipeline.h"
#include "compile/translators/translator_factory.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "plan/operator/operator.h"

namespace kush::compile {

execution::ExecutableQuery TranslateQuery(const plan::Operator& op) {
  khir::ProgramBuilder program_builder;
  execution::PipelineBuilder pipeline_builder;
  execution::QueryState state;

  ForwardDeclare(program_builder);

  // Generate code for operator
  TranslatorFactory factory(program_builder, pipeline_builder, state);
  auto translator = factory.Compute(op);

  proxy::Pipeline output_pipeline(program_builder, pipeline_builder);
  translator->Produce(output_pipeline);
  output_pipeline.Build();

  return execution::ExecutableQuery(
      std::move(translator), program_builder.Build(),
      std::move(pipeline_builder), std::move(state));
}

}  // namespace kush::compile