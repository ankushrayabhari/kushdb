#include "execution/pipeline.h"

#include <memory>
#include <string>
#include <vector>

#include "util/vector_util.h"

namespace kush::execution {

Pipeline::Pipeline(int id) : id_(id) {}

std::string Pipeline::Name() const { return "pipeline" + std::to_string(id_); }

void Pipeline::AddPredecessor(std::unique_ptr<Pipeline> pred) {
  predecessors_.push_back(std::move(pred));
}

int PipelineBuilder::id_ = 0;

std::vector<std::reference_wrapper<const Pipeline>> Pipeline::Predecessors()
    const {
  return util::ImmutableReferenceVector(predecessors_);
}

Pipeline& PipelineBuilder::CreatePipeline() {
  pipelines_.push(std::make_unique<Pipeline>(id_++));
  return *pipelines_.top();
}

Pipeline& PipelineBuilder::GetCurrentPipeline() { return *pipelines_.top(); }

std::unique_ptr<Pipeline> PipelineBuilder::FinishPipeline() {
  auto ret = std::move(pipelines_.top());
  pipelines_.pop();
  return ret;
}

}  // namespace kush::execution