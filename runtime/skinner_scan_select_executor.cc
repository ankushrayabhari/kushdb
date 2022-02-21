#include "runtime/skinner_scan_select_executor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"

ABSL_FLAG(int32_t, scan_select_budget_per_episode, 10000,
          "Scan/Select budget per episode");

namespace kush::runtime {

class ScanSelectState {
 public:
  ScanSelectState(int32_t cardinality)
      : cardinality_(cardinality), last_completed_tuple_(-1) {}

  bool IsComplete() { return last_completed_tuple_ == cardinality_; }

  std::optional<int32_t> GetLastCompletedTupleIdx() {
    return last_completed_tuple_;
  }

  void Update(int32_t last_completed_tuple) {
    if (last_completed_tuple < last_completed_tuple_) {
      throw std::runtime_error("Negative progress was made.");
    }

    last_completed_tuple_ = last_completed_tuple;
  }

 private:
  int32_t cardinality_;
  int32_t last_completed_tuple_;
};

class ScanSelectEnvironment {
 public:
  virtual double Execute(bool index, const std::vector<int>& order) = 0;

  int32_t ComputeLastCompletedTuple(int last_completed_tuple, int status) {
    auto should_decrement = status == -2;
    if (should_decrement) {
      last_completed_tuple--;
    }

    return last_completed_tuple;
  }

  double Reward(int32_t initial_last_completed_tuple,
                int32_t final_last_completed_tuple, int32_t cardinality) {
    auto weight = 1.0 / (cardinality - initial_last_completed_tuple);
    return (final_last_completed_tuple - initial_last_completed_tuple) / weight;
  }
};

struct PermutableScanSelectExecutionEngineFlags {
  int32_t* idx;
  int32_t* num_handlers;
  std::add_pointer<int32_t()>::type* handlers;
};

class PermutableScanSelectEnvironment : public ScanSelectEnvironment {
 public:
  PermutableScanSelectEnvironment(
      int32_t cardinality,
      const std::add_pointer<int32_t(int32_t, int8_t)>::type index_scan_fn,
      const std::add_pointer<int32_t(int32_t, int8_t)>::type scan_fn,
      const std::vector<std::add_pointer<int32_t()>::type> predicate_fn,
      PermutableScanSelectExecutionEngineFlags execution_engine)
      : cardinality_(cardinality),
        budget_per_episode_(FLAGS_scan_select_budget_per_episode.Get()),
        index_scan_fn_(index_scan_fn),
        scan_fn_(scan_fn),
        predicate_fn_(predicate_fn),
        state_(cardinality),
        execution_engine_(execution_engine) {}

  bool IsComplete() { return state_.IsComplete(); }

  double Execute(bool index, const std::vector<int>& order) override {
    auto initial_last_completed_tuple = state_.GetLastCompletedTupleIdx();
    auto base = SetExecutionEngine(index, order, initial_last_completed_tuple);
    auto status = base(budget_per_episode_,
                       initial_last_completed_tuple.has_value() ? 1 : 0);

    auto final_last_completed_tuple =
        ComputeLastCompletedTuple(*execution_engine_.idx, status);

    state_.Update(final_last_completed_tuple);

    return Reward(initial_last_completed_tuple.value_or(0),
                  final_last_completed_tuple, cardinality_);
  }

 private:
  std::add_pointer<int(int, int8_t)>::type SetExecutionEngine(
      bool index, const std::vector<int>& orders,
      std::optional<int32_t> initial_last_completed_tuple) {
    *execution_engine_.idx = initial_last_completed_tuple.value_or(-1);
    *execution_engine_.num_handlers = orders.size();

    int handler_pos = 0;
    for (int p : orders) {
      execution_engine_.handlers[handler_pos++] = predicate_fn_[p];
    }

    return index ? index_scan_fn_ : scan_fn_;
  }

  int32_t cardinality_;
  int32_t budget_per_episode_;
  const std::add_pointer<int32_t(int32_t, int8_t)>::type index_scan_fn_;
  const std::add_pointer<int32_t(int32_t, int8_t)>::type scan_fn_;
  const std::vector<std::add_pointer<int32_t()>::type> predicate_fn_;
  ScanSelectState state_;
  PermutableScanSelectExecutionEngineFlags execution_engine_;
};

/*
progress sharing
- basically options for base table are:

- scan over any index
- loop

- after that you have any remaining predicates

- for each indexed predicate, generate a function in the same manner

function():
    fast forward each idx to last_tuple
    loop over intersection
      loop over handlers
        handler

function()
    loop over base table
      loop over handlers
          handler

function():
    evaluate predicate
    handler()
*/

void ExecutePermutableSkinnerScanSelect(
    int num_predicates, const std::vector<int32_t>* indexed_predicates,
    std::add_pointer<int32_t(int32_t, int8_t)>::type index_scan_fn,
    std::add_pointer<int32_t(int32_t, int8_t)>::type scan_fn,
    int32_t* num_handlers, std::add_pointer<int32_t()>::type* handlers,
    int32_t* idx) {
  int32_t cardinality = *idx;
  PermutableScanSelectExecutionEngineFlags execution_engine{
      .idx = idx, .num_handlers = num_handlers, .handlers = handlers};
  std::vector<std::add_pointer<int32_t()>::type> predicate_fn;
  for (int i = 0; i < num_predicates; i++) {
    predicate_fn.push_back(handlers[i]);
  }

  PermutableScanSelectEnvironment environment(
      cardinality, index_scan_fn, scan_fn, predicate_fn, execution_engine);
}

}  // namespace kush::runtime