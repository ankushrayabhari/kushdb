#include "execution/pipeline.h"

#include <memory>
#include <string>
#include <vector>

#include "util/vector_util.h"

namespace kush::execution {

Pipeline::Pipeline(int id) : id_(id), split_(false) {}

void Pipeline::SetDriver(Pipeline& pred) {
  driver_ = pred.id_;
  AddPredecessor(pred);
}

void Pipeline::AddPredecessor(Pipeline& pred) {
  pred_.push_back(pred.id_);
  pred.succ_.push_back(id_);
}

std::string Pipeline::InitName() const { return "init_" + std::to_string(id_); }

std::string Pipeline::BodyName() const { return "body_" + std::to_string(id_); }

std::string Pipeline::ResetName() const {
  return "reset_" + std::to_string(id_);
}

bool Pipeline::Split() const { return split_; }

void Pipeline::SetSplit(bool s) { split_ = s; }

std::string Pipeline::SizeName() const { return "size_" + std::to_string(id_); }

const std::vector<int>& Pipeline::Successors() const { return succ_; }

const std::vector<int>& Pipeline::Predecessors() const { return pred_; }

std::optional<int> Pipeline::Driver() const { return driver_; }

PipelineBuilder::PipelineBuilder() : id_(0) {}

Pipeline& PipelineBuilder::CreatePipeline() {
  pipelines_.push_back(std::make_unique<Pipeline>(id_++));
  return *pipelines_.back();
}

std::vector<std::reference_wrapper<const Pipeline>> PipelineBuilder::Pipelines()
    const {
  return util::ImmutableReferenceVector(pipelines_);
}

}  // namespace kush::execution