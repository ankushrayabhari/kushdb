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
ABSL_FLAG(bool, scan_select_forget, false,
          "Scan/Select forget learned info periodically.");

namespace kush::runtime {

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

class ScanSelectUctNode {
 public:
  ScanSelectUctNode(int round_ctr, int num_predicates, bool can_index,
                    const std::vector<int32_t>& indexed_predicates,
                    ScanSelectEnvironment& environment)
      : environment_(environment),
        indexed_predicates_(indexed_predicates),
        created_in_(round_ctr),
        tree_level_(0),
        num_predicates_(num_predicates),
        num_actions_(can_index ? 2 : 1),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    for (int i = 0; i < num_predicates_; i++) {
      unevaluated_predicates_.push_back(i);
    }

    if (can_index) {
      priority_actions_ = {1, 0};
    } else {
      priority_actions_ = {0};
    }
  }

  ScanSelectUctNode(int round_ctr, ScanSelectUctNode& parent,
                    const std::vector<int>& evaluated_predicates)
      : environment_(parent.environment_),
        indexed_predicates_(parent.indexed_predicates_),
        created_in_(round_ctr),
        tree_level_(parent.tree_level_ + 1),
        num_predicates_(parent.num_predicates_),
        num_actions_(parent.unevaluated_predicates_.size() -
                     evaluated_predicates.size()),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        predicate_per_action_(num_actions_),
        evaluated_predicates_(parent.evaluated_predicates_),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    evaluated_predicates_.insert(evaluated_predicates.begin(),
                                 evaluated_predicates.end());
    int action = 0;
    for (int i = 0; i < num_predicates_; i++) {
      if (!evaluated_predicates_.contains(i)) {
        unevaluated_predicates_.push_back(i);
        priority_actions_.push_back(action);
        predicate_per_action_[action++] = i;
      }
    }
  }

  double Sample(int round_ctr, bool& use_index, std::vector<int>& order) {
    if (num_actions_ == 0) {
      return environment_.Execute(use_index, order);
    }

    int action = SelectAction();

    std::vector<int> evaluated_predicates;
    if (tree_level_ == 0) {
      use_index = action != 0;

      if (use_index) {
        evaluated_predicates = indexed_predicates_;
      }
    } else {
      int predicate = predicate_per_action_[action];
      order.push_back(predicate);
      evaluated_predicates.push_back(predicate);
    }

    bool can_expand = created_in_ != round_ctr;
    if (can_expand && child_nodes_[action] == nullptr) {
      child_nodes_[action] = std::make_unique<ScanSelectUctNode>(
          round_ctr, *this, evaluated_predicates);
    }

    ScanSelectUctNode* child = child_nodes_[action].get();

    double reward = child != nullptr
                        ? child->Sample(round_ctr, use_index, order)
                        : Playout(use_index, order);

    UpdateStatistics(action, reward);
    return reward;
  }

 private:
  double Playout(bool& use_index, std::vector<int>& order) {
    absl::btree_set<int> current_evaluated;
    std::vector<int> current_unevaluated;
    if (tree_level_ == 0) {
      if (use_index) {
        current_evaluated.insert(indexed_predicates_.begin(),
                                 indexed_predicates_.end());
      }
    } else {
      current_evaluated.insert(evaluated_predicates_.begin(),
                               evaluated_predicates_.end());
      current_evaluated.insert(order.back());
    }

    for (int i = 0; i < num_predicates_; i++) {
      if (!current_evaluated.contains(i)) {
        current_unevaluated.push_back(i);
      }
    }

    std::shuffle(current_unevaluated.begin(), current_unevaluated.end(), rng_);
    for (int i = 0; i < current_unevaluated.size(); i++) {
      int predicate = current_unevaluated[i];
      order.push_back(predicate);
    }
    return environment_.Execute(use_index, order);
  }

  int SelectAction() {
    if (!priority_actions_.empty()) {
      int num_untried = priority_actions_.size();

      int action_idx = rng_() % num_untried;
      if (tree_level_ == 0) {
        action_idx = 0;
      }

      int action = priority_actions_[action_idx];
      priority_actions_.erase(priority_actions_.begin() + action_idx);
      return action;
    }

    int offset = rng_() % num_actions_;
    int best_action = -1;
    double best_quality = -1;
    for (int action_idx = 0; action_idx < num_actions_; ++action_idx) {
      int action = (offset + action_idx) % num_actions_;

      double mean_reward =
          acc_reward_per_action_[action] / num_tries_per_action_[action];
      double exploration =
          std::sqrt(std::log(num_visits_) / num_tries_per_action_[action]);

      double quality = mean_reward + EXPLORATION_WEIGHT_ * exploration;
      if (quality > best_quality) {
        best_action = action;
        best_quality = quality;
      }
    }
    assert(best_action >= 0);
    return best_action;
  }

  void UpdateStatistics(int action, double reward) {
    num_visits_++;
    num_tries_per_action_[action]++;
    acc_reward_per_action_[action] += reward;
    assert(acc_reward_per_action_[action] >= 0);
  }

 private:
  ScanSelectEnvironment& environment_;
  const std::vector<int32_t>& indexed_predicates_;
  int created_in_;
  int tree_level_;
  int num_predicates_;
  int num_actions_;
  std::vector<std::unique_ptr<ScanSelectUctNode>> child_nodes_;
  int num_visits_;
  std::vector<int> priority_actions_;
  std::vector<int> num_tries_per_action_;
  std::vector<double> acc_reward_per_action_;
  std::vector<int> predicate_per_action_;
  absl::btree_set<int> evaluated_predicates_;
  std::vector<int> unevaluated_predicates_;
  std::default_random_engine rng_;

  static constexpr double EXPLORATION_WEIGHT_ = 1E-5;
};

class UctScanSelectAgent {
 public:
  UctScanSelectAgent(int num_predicates, bool can_index,
                     const std::vector<int32_t>& indexed_predicates,
                     ScanSelectEnvironment& environment)
      : environment_(environment),
        round_ctr_(0),
        num_predicates_(num_predicates),
        can_index_(can_index),
        indexed_predicates_(indexed_predicates),
        should_forget_(FLAGS_scan_select_forget.Get()),
        next_forget_(10),
        root_(std::make_unique<ScanSelectUctNode>(
            round_ctr_, num_predicates_, can_index_, indexed_predicates_,
            environment_)) {}

  void Act() {
    round_ctr_++;
    bool use_index;
    std::vector<int> order;
    order.reserve(num_predicates_);
    root_->Sample(round_ctr_, use_index, order);

    // Consider memory loss
    if (should_forget_ && round_ctr_ == next_forget_) {
      root_ = std::make_unique<ScanSelectUctNode>(
          round_ctr_, num_predicates_, can_index_, indexed_predicates_,
          environment_);
      next_forget_ *= 10;
    }
  }

 private:
  ScanSelectEnvironment& environment_;
  int round_ctr_;
  int num_predicates_;
  bool can_index_;
  const std::vector<int32_t>& indexed_predicates_;
  bool should_forget_;
  int64_t next_forget_;
  std::unique_ptr<ScanSelectUctNode> root_;
};

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

  bool can_index = index_scan_fn == nullptr;
  UctScanSelectAgent agent(num_predicates, can_index, *indexed_predicates,
                           environment);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

}  // namespace kush::runtime