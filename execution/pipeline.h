#pragma once

#include <memory>
#include <string>
#include <vector>

namespace kush::execution {

class Pipeline {
 public:
  Pipeline(int id);
  void AddPredecessor(std::unique_ptr<Pipeline> pred);

  std::vector<std::reference_wrapper<const Pipeline>> Predecessors() const;
  std::string Name() const;

 private:
  int id_;
  std::vector<std::unique_ptr<Pipeline>> predecessors_;
};

class PipelineBuilder {
 public:
  Pipeline& CreatePipeline();
  std::unique_ptr<Pipeline> FinishPipeline();

 private:
  std::unique_ptr<Pipeline> pipeline_;
  int id_;
};

}  // namespace kush::execution