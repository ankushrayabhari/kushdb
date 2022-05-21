#include "execution/executable_query.h"

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/backend.h"

namespace kush::execution {

ExecutableQuery::ExecutableQuery(
    std::unique_ptr<compile::OperatorTranslator> translator,
    std::unique_ptr<khir::Backend> program, PipelineBuilder pipelines,
    QueryState state)
    : translator_(std::move(translator)),
      program_(std::move(program)),
      pipelines_(std::move(pipelines)),
      state_(std::move(state)) {}

void ExecutableQuery::Compile() { program_->Compile(); }

void TopSortVisit(
    int curr, std::vector<bool>& visited,
    std::vector<std::reference_wrapper<const kush::execution::Pipeline>>
        pipelines,
    std::vector<int>& order) {
  visited[curr] = true;

  for (auto next : pipelines[curr].get().Successors()) {
    if (!visited[next]) {
      TopSortVisit(next, visited, pipelines, order);
    }
  }

  order.push_back(curr);
}

using init_fn = std::add_pointer<void()>::type;
using reset_fn = std::add_pointer<void()>::type;
using split_body_fn = std::add_pointer<void(int32_t, int32_t)>::type;
using body_fn = std::add_pointer<void()>::type;
using size_fn = std::add_pointer<int32_t()>::type;

int32_t GetInputSize(
    int curr,
    std::vector<std::reference_wrapper<const kush::execution::Pipeline>>
        pipelines,
    khir::Backend& program) {
  int32_t input_size = 0;
  const auto& pipeline = pipelines[curr].get();
  if (pipeline.Driver().has_value()) {
    auto pred = pipeline.Driver().value();
    auto size = reinterpret_cast<size_fn>(
        program.GetFunction(pipelines[pred].get().SizeName()));
    input_size = size();
  }
  return input_size;
}

void InitializeOutput(
    int curr,
    std::vector<std::reference_wrapper<const kush::execution::Pipeline>>
        pipelines,
    khir::Backend& program) {
  const auto& pipeline = pipelines[curr].get();
  auto init =
      reinterpret_cast<init_fn>(program.GetFunction(pipeline.InitName()));
  init();
}

void CleanUpPredecessors(
    int curr,
    std::vector<std::reference_wrapper<const kush::execution::Pipeline>>
        pipelines,
    std::vector<int>& users, khir::Backend& program) {
  const auto& pipeline = pipelines[curr].get();
  for (auto pred : pipeline.Predecessors()) {
    auto num_users = pipelines[pred].get().Successors().size();
    if (++users[pred] == num_users) {
      auto reset = reinterpret_cast<reset_fn>(
          program.GetFunction(pipelines[pred].get().ResetName()));
      reset();
    }
  }
}

constexpr int32_t CHUNK_SIZE = 1 << 14;

void ExecutableQuery::Execute() {
  auto pipelines = pipelines_.Pipelines();

  // generate topological ordering of pipelines
  std::vector<bool> visited(pipelines.size(), false);
  std::vector<int> order;
  for (int i = 0; i < pipelines.size(); i++) {
    if (!visited[i]) {
      TopSortVisit(i, visited, pipelines, order);
    }
  }
  std::reverse(order.begin(), order.end());

  std::vector<int> users(pipelines.size(), 0);

  // execute each pipeline in topological order
  for (int i : order) {
    InitializeOutput(i, pipelines, *program_);

    const auto& pipeline = pipelines[i].get();
    if (pipeline.Split()) {
      auto input_size = GetInputSize(i, pipelines, *program_);
      auto body = reinterpret_cast<split_body_fn>(
          program_->GetFunction(pipelines[i].get().BodyName()));
      int32_t next_tuple = 0;
      while (next_tuple < input_size) {
        auto start = next_tuple;
        auto end = std::min(next_tuple + CHUNK_SIZE - 1, input_size - 1);
        body(start, end);
        next_tuple = end + 1;
      }
    } else {
      auto body = reinterpret_cast<body_fn>(
          program_->GetFunction(pipelines[i].get().BodyName()));
      body();
    }

    CleanUpPredecessors(i, pipelines, users, *program_);
  }

  {  // Clean up the final buffer
    auto final = order.back();
    if (!pipelines[final].get().Successors().empty()) {
      throw std::runtime_error("A pipeline references the output pipeline");
    }
    auto reset = reinterpret_cast<reset_fn>(
        program_->GetFunction(pipelines[final].get().ResetName()));
    reset();
  }
}

}  // namespace kush::execution