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
                  PipelineBuilder pipelines, QueryState state);
  void Compile();
  void Execute();

 private:
  std::unique_ptr<compile::OperatorTranslator> translator_;
  std::unique_ptr<khir::Backend> program_;
  PipelineBuilder pipelines_;
  QueryState state_;
};

}  // namespace kush::execution