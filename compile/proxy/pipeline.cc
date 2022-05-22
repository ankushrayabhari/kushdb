#include "compile/proxy/pipeline.h"

#include <functional>
#include <optional>
#include <vector>

#include "khir/program_builder.h"

namespace kush::compile::proxy {

Pipeline::Pipeline(khir::ProgramBuilder& program,
                   execution::PipelineBuilder& pipeline_builder)
    : program_(program),
      pipeline_(pipeline_builder.CreatePipeline()),
      init_(false),
      reset_(false),
      size_(false),
      body_(false) {}

void Pipeline::Init(std::function<void()> init) {
  init_ = true;
  program_.CreateNamedFunction(program_.VoidType(), {}, pipeline_.InitName());
  init();
  program_.Return();
}

void Pipeline::Reset(std::function<void()> reset) {
  reset_ = true;
  program_.CreateNamedFunction(program_.VoidType(), {}, pipeline_.ResetName());
  reset();
  program_.Return();
}

void Pipeline::Size(std::function<Int32()> size) {
  size_ = true;
  program_.CreateNamedFunction(program_.I32Type(), {}, pipeline_.SizeName());
  auto value = size();
  program_.Return(value.Get());
}

void Pipeline::Body(Pipeline& pipeline,
                    std::function<void(Int32, Int32)> body) {
  body_ = true;
  this->Get().SetDriver(pipeline.Get());
  pipeline_.SetSplit(true);
  auto func = program_.CreateNamedFunction(
      program_.VoidType(), {program_.I32Type(), program_.I32Type()},
      pipeline_.BodyName());
  auto args = program_.GetFunctionArguments(func);
  body(Int32(program_, args[0]), Int32(program_, args[1]));
  program_.Return();
}

void Pipeline::Body(std::function<void()> body) {
  body_ = true;
  pipeline_.SetSplit(false);
  program_.CreateNamedFunction(program_.VoidType(), {}, pipeline_.BodyName());
  body();
  program_.Return();
}

void Pipeline::Build() {
  if (!init_) {
    Init([]() {});
  }
  if (!reset_) {
    Reset([]() {});
  }
  if (!size_) {
    Size([&]() { return Int32(program_, 0); });
  }
  if (!body_) {
    Body([]() {});
  }
}

execution::Pipeline& Pipeline::Get() { return pipeline_; }

}  // namespace kush::compile::proxy