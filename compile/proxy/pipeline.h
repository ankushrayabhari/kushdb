#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "compile/proxy/value/ir_value.h"
#include "execution/pipeline.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class Pipeline {
 public:
  Pipeline(khir::ProgramBuilder& program,
           execution::PipelineBuilder& pipeline_builder);

  void Init(std::function<void()> init);
  void Reset(std::function<void()> reset);
  void Size(std::function<Int32()> size);

  void Body(Pipeline& pipeline, std::function<void(Int32, Int32)> body);
  void Body(std::function<void()> body);

  void Build();

  execution::Pipeline& Get();

 private:
  khir::ProgramBuilder& program_;
  execution::Pipeline& pipeline_;
  bool init_;
  bool reset_;
  bool size_;
  bool body_;
};

}  // namespace kush::compile::proxy