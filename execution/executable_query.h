#pragma once

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/backend.h"

namespace kush::execution {

class ExecutableQuery {
 public:
  ExecutableQuery(std::unique_ptr<compile::OperatorTranslator> translator,
                  khir::Program program, PipelineBuilder pipelines,
                  QueryState state);
  void Execute();

 private:
  std::unique_ptr<compile::OperatorTranslator> translator_;
  khir::Program program_;
  PipelineBuilder pipelines_;
  QueryState state_;
};

}  // namespace kush::execution