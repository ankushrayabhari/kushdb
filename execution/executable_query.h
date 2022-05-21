#pragma once

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/backend.h"

namespace kush::execution {

class ExecutableQuery {
 public:
  ExecutableQuery(std::unique_ptr<compile::OperatorTranslator> translator,
                  std::unique_ptr<khir::Backend> program,
                  std::unique_ptr<const Pipeline> output_pipeline,
                  QueryState state);

  void Compile();
  void Execute();

 private:
  void ExecutePipeline(const Pipeline& pipeline);
  std::unique_ptr<compile::OperatorTranslator> translator_;
  std::unique_ptr<khir::Backend> program_;
  std::unique_ptr<const Pipeline> output_pipeline_;
  QueryState state_;
};

}  // namespace kush::execution