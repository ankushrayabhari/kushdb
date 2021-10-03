#pragma once

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/program.h"

namespace kush::execution {

class ExecutableQuery {
 public:
  ExecutableQuery(std::unique_ptr<compile::OperatorTranslator> translator,
                  std::unique_ptr<khir::Program> program,
                  std::unique_ptr<const Pipeline> output_pipeline);

  void Compile();
  void Execute();

 private:
  void ExecutePipeline(const Pipeline& pipeline);
  std::unique_ptr<compile::OperatorTranslator> translator_;
  std::unique_ptr<khir::Program> program_;
  std::unique_ptr<const Pipeline> output_pipeline_;
};

}  // namespace kush::execution