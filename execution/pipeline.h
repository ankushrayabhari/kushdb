#pragma once

#include <memory>
#include <stack>
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
  Pipeline& GetCurrentPipeline();
  std::unique_ptr<Pipeline> FinishPipeline();

 private:
  std::stack<std::unique_ptr<Pipeline>> pipelines_;
  static int id_;
};

}  // namespace kush::execution