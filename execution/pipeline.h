#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kush::execution {

class Pipeline {
 public:
  Pipeline(int id);
  std::string InitName() const;
  std::string BodyName() const;
  std::string SizeName() const;
  std::string ResetName() const;

  void SetDriver(Pipeline& rhs);
  std::optional<int> Driver() const;

  void AddPredecessor(Pipeline& rhs);
  const std::vector<int>& Predecessors() const;
  const std::vector<int>& Successors() const;

  bool Split() const;
  void SetSplit(bool s);

 private:
  int id_;
  std::optional<int> driver_;
  std::vector<int> succ_;
  std::vector<int> pred_;
  bool split_;
};

class PipelineBuilder {
 public:
  PipelineBuilder();
  Pipeline& CreatePipeline();

  std::vector<std::reference_wrapper<const Pipeline>> Pipelines() const;

 private:
  std::vector<std::unique_ptr<Pipeline>> pipelines_;
  int id_;
};

}  // namespace kush::execution