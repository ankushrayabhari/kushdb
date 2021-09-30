#include "execution/pipeline.h"

#include <memory>
#include <string>
#include <vector>

#include "util/vector_util.h"

namespace kush::execution {

Pipeline::Pipeline(int id) : id_(id) {}

std::string Pipeline::GetPipelineName() const {
  return "pipeline" + std::to_string(id_);
}

void Pipeline::AddPredecessor(std::unique_ptr<Pipeline> pred) {
  predecessors_.push_back(std::move(pred));
}

std::vector<std::reference_wrapper<const Pipeline>> Pipeline::Predecessors()
    const {
  return util::ImmutableReferenceVector(predecessors_);
}

Pipeline& PipelineBuilder::CreatePipeline() {
  pipeline_ = std::make_unique<Pipeline>(id_++);
  return *pipeline_;
}

std::unique_ptr<Pipeline> PipelineBuilder::FinishPipeline() {
  auto ret = std::move(pipeline_);
  pipeline_.reset();
  return ret;
}

}  // namespace kush::execution