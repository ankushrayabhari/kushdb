#include "execution/executable_query.h"

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/backend.h"

namespace kush::execution {

ExecutableQuery::ExecutableQuery(
    std::unique_ptr<compile::OperatorTranslator> translator,
    std::unique_ptr<khir::Backend> program,
    std::unique_ptr<const Pipeline> output_pipeline, QueryState state)
    : translator_(std::move(translator)),
      program_(std::move(program)),
      output_pipeline_(std::move(output_pipeline)),
      state_(std::move(state)) {}

void ExecutableQuery::Compile() { program_->Compile(); }

void ExecutableQuery::ExecutePipeline(const Pipeline& pipeline) {
  for (auto predecessor : pipeline.Predecessors()) {
    ExecutePipeline(predecessor.get());
  }

  using compute_fn = std::add_pointer<void()>::type;
  auto compute =
      reinterpret_cast<compute_fn>(program_->GetFunction(pipeline.Name()));
  compute();
}

void ExecutableQuery::Execute() {
  // execute each of the child pipelines
  ExecutePipeline(*output_pipeline_);
}

}  // namespace kush::execution