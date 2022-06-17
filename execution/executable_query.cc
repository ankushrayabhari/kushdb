#include "execution/executable_query.h"

#include <iostream>

#include "absl/flags/flag.h"

#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "khir/asm/asm_backend.h"
#include "khir/asm/reg_alloc_impl.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"

ABSL_FLAG(std::string, pipeline_mode, "adaptive",
          "Pipeline Mode: static/adaptive.");

namespace kush::execution {

enum class PipelineMode { STATIC, ADAPTIVE };

ExecutableQuery::ExecutableQuery(
    std::unique_ptr<compile::OperatorTranslator> translator,
    std::unique_ptr<khir::Program> program, PipelineBuilder pipelines,
    QueryState state)
    : translator_(std::move(translator)),
      program_(std::move(program)),
      pipelines_(std::move(pipelines)),
      state_(std::move(state)) {}

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

constexpr int32_t CHUNK_SIZE = 1 << 13;

void ExecutableQuery::Execute() {
  PipelineMode mode;
  if (FLAGS_pipeline_mode.CurrentValue() == "static") {
    mode = PipelineMode::STATIC;
  } else if (FLAGS_pipeline_mode.CurrentValue() == "adaptive") {
    mode = PipelineMode::ADAPTIVE;
  } else {
    throw std::runtime_error("Unknown pipeline mode.");
  }

  std::unique_ptr<khir::Backend> backend;
  std::unique_ptr<khir::Backend> optimized_backend;
  auto backend_type = khir::GetBackendType();
  if (mode == PipelineMode::ADAPTIVE) {
    backend_type = khir::BackendType::ASM;
  }
  switch (backend_type) {
    case khir::BackendType::ASM: {
      auto b = std::make_unique<khir::ASMBackend>(*program_,
                                                  khir::GetRegAllocImpl());
      b->Compile();
      backend = std::move(b);
      break;
    }

    case khir::BackendType::LLVM: {
      backend = std::make_unique<khir::LLVMBackend>(*program_);
      break;
    }
  }
  if (mode == PipelineMode::ADAPTIVE) {
    optimized_backend = std::make_unique<khir::LLVMBackend>(*program_);
  }

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
    InitializeOutput(i, pipelines, *backend);

    const auto& pipeline = pipelines[i].get();
    if (pipeline.Split()) {
      auto input_size = GetInputSize(i, pipelines, *backend);
      auto body = reinterpret_cast<split_body_fn>(
          backend->GetFunction(pipelines[i].get().BodyName()));

      if (mode == PipelineMode::ADAPTIVE) {
        int count = 0;
        int THRESHOLD = 2;
        double tot = 0;

        int32_t next_tuple = 0;
        while (next_tuple < input_size && count < THRESHOLD) {
          auto start = next_tuple;
          auto end = std::min(next_tuple + CHUNK_SIZE - 1, input_size - 1);

          next_tuple = end + 1;

          auto t1 = std::chrono::high_resolution_clock::now();
          body(start, end);
          auto t2 = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
          tot += fp_ms.count();
          count++;
        }

        if (next_tuple < input_size) {
          auto time_per_morsel = tot / THRESHOLD;
          // estimate a 1.2x speedup
          // estimate comp time at 10ms?
          auto opt_time_per_morsel = time_per_morsel / 1.2;
          auto num_morsels_left = (input_size - next_tuple) / CHUNK_SIZE;

          auto duration_unoptimized = time_per_morsel * num_morsels_left;
          auto duration_optimized = opt_time_per_morsel * num_morsels_left + 10;

          auto exec = body;
          if (duration_optimized < duration_unoptimized) {
            exec = reinterpret_cast<split_body_fn>(
                optimized_backend->GetFunction(pipelines[i].get().BodyName()));
          }

          while (next_tuple < input_size) {
            auto start = next_tuple;
            auto end = std::min(next_tuple + CHUNK_SIZE - 1, input_size - 1);
            exec(start, end);
            next_tuple = end + 1;
          }
        }
      } else {
        int32_t next_tuple = 0;
        while (next_tuple < input_size) {
          auto start = next_tuple;
          auto end = std::min(next_tuple + CHUNK_SIZE - 1, input_size - 1);
          body(start, end);
          next_tuple = end + 1;
        }
      }

    } else {
      auto body = reinterpret_cast<body_fn>(
          backend->GetFunction(pipelines[i].get().BodyName()));
      body();
    }

    CleanUpPredecessors(i, pipelines, users, *backend);
  }

  {  // Clean up the final buffer
    auto final = order.back();
    if (!pipelines[final].get().Successors().empty()) {
      throw std::runtime_error("A pipeline references the output pipeline");
    }
    auto reset = reinterpret_cast<reset_fn>(
        backend->GetFunction(pipelines[final].get().ResetName()));
    reset();
  }
}

}  // namespace kush::execution