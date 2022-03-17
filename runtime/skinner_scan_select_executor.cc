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

struct PermutableScanSelectExecutionEngineFlags {
  int32_t* idx;
  int32_t* index_array;
  int32_t* index_array_size;
  std::add_pointer<bool()>::type* predicate_arr;
};

class ScanSelectEnvironment {
 public:
  virtual double Execute(const std::vector<int>& index_order,
                         const std::vector<int>& orders) = 0;

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

class IndexUCTNode;

class PredicateOrderUCTNode {
 public:
  PredicateOrderUCTNode(int round_ctr, IndexUCTNode& parent);

  PredicateOrderUCTNode(int round_ctr, PredicateOrderUCTNode& parent,
                        int predicate)
      : environment_(parent.environment_),
        created_in_(round_ctr),
        num_predicates_(parent.num_predicates_),
        num_actions_(num_predicates_ - parent.evaluated_predicates_.size() - 1),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        predicate_per_action_(num_actions_),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    evaluated_predicates_.insert(parent.evaluated_predicates_.begin(),
                                 parent.evaluated_predicates_.end());
    evaluated_predicates_.insert(predicate);
    int action = 0;
    for (int i = 0; i < num_predicates_; i++) {
      if (!evaluated_predicates_.contains(i)) {
        unevaluated_predicates_.push_back(i);
        priority_actions_.push_back(action);
        predicate_per_action_[action++] = i;
      }
    }
  }

  double Sample(int round_ctr, std::vector<int>& index_order,
                std::vector<int>& order) {
    if (num_actions_ == 0) {
      return environment_.Execute(index_order, order);
    }

    int action = SelectAction();
    int predicate = predicate_per_action_[action];
    order.push_back(predicate);

    bool can_expand = created_in_ != round_ctr;
    if (can_expand && child_nodes_[action] == nullptr) {
      child_nodes_[action] =
          std::make_unique<PredicateOrderUCTNode>(round_ctr, *this, predicate);
    }

    PredicateOrderUCTNode* child = child_nodes_[action].get();

    double reward = child != nullptr
                        ? child->Sample(round_ctr, index_order, order)
                        : Playout(index_order, order);

    UpdateStatistics(action, reward);
    return reward;
  }

 private:
  double Playout(std::vector<int>& index_order, std::vector<int>& order) {
    std::vector<int> current_unevaluated;
    for (int i = 0; i < num_predicates_; i++) {
      if (evaluated_predicates_.contains(i) || order.back() == i) {
        continue;
      }
      current_unevaluated.push_back(i);
    }

    std::shuffle(current_unevaluated.begin(), current_unevaluated.end(), rng_);
    for (int i = 0; i < current_unevaluated.size(); i++) {
      int predicate = current_unevaluated[i];
      order.push_back(predicate);
    }

    return environment_.Execute(index_order, order);
  }

  int SelectAction() {
    if (!priority_actions_.empty()) {
      int num_untried = priority_actions_.size();
      int action_idx = rng_() % num_untried;
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

  ScanSelectEnvironment& environment_;
  int created_in_;
  int num_predicates_;
  int num_actions_;
  std::vector<std::unique_ptr<PredicateOrderUCTNode>> child_nodes_;
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

class IndexUCTNode {
 public:
  IndexUCTNode(int round_ctr, ScanSelectEnvironment& environment,
               int num_predicates, const std::vector<int>& index_predicates)
      : environment_(environment),
        created_in_(round_ctr),
        num_predicates_(num_predicates),
        index_predicates_(index_predicates),
        current_idx_(0),
        num_actions_(current_idx_ == index_predicates_.size() ? 1 : 2),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        predicate_per_action_(num_actions_),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    // action 0 is not selecting index_predicates[0]
    // action 1 is selecting index_predicates[0]
    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
    }
  }

  IndexUCTNode(int round_ctr, IndexUCTNode& parent, bool selected)
      : environment_(parent.environment_),
        created_in_(round_ctr),
        num_predicates_(parent.num_predicates_),
        index_predicates_(parent.index_predicates_),
        current_idx_(parent.current_idx_ + 1),
        num_actions_(current_idx_ == index_predicates_.size() ? 1 : 2),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        predicate_per_action_(num_actions_),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    evaluated_predicates_.insert(parent.evaluated_predicates_.begin(),
                                 parent.evaluated_predicates_.end());
    if (selected) {
      evaluated_predicates_.insert(index_predicates_[parent.current_idx_]);
    }

    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
    }
  }

  double Sample(int round_ctr, std::vector<int>& index_order,
                std::vector<int>& order) {
    bool can_expand = created_in_ != round_ctr;
    if (num_actions_ == 1) {
      if (can_expand && predicate_order_node_ == nullptr) {
        predicate_order_node_ =
            std::make_unique<PredicateOrderUCTNode>(round_ctr, *this);
      }

      double reward =
          predicate_order_node_ != nullptr
              ? predicate_order_node_->Sample(round_ctr, index_order, order)
              : Playout(index_order, order);
      UpdateStatistics(0, reward);
      return reward;
    } else {
      int action = SelectAction();

      bool selected = action == 1;
      if (selected) {
        index_order.push_back(index_predicates_[current_idx_]);
      }

      if (can_expand && child_nodes_[action] == nullptr) {
        child_nodes_[action] =
            std::make_unique<IndexUCTNode>(round_ctr, *this, selected);
      }

      IndexUCTNode* child = child_nodes_[action].get();
      double reward = child != nullptr
                          ? child->Sample(round_ctr, index_order, order)
                          : Playout(index_order, order);

      UpdateStatistics(action, reward);
      return reward;
    }
  }

 private:
  double Playout(std::vector<int>& index_order, std::vector<int>& order) {
    auto dist = std::uniform_int_distribution<int>(0, 1);
    for (int i = current_idx_ + 1; i < index_predicates_.size(); i++) {
      if (dist(rng_)) {
        index_order.push_back(index_predicates_[i]);
      }
    }

    absl::flat_hash_set<int> evaluated_predicates(index_order.begin(),
                                                  index_order.end());
    for (int i = 0; i < num_predicates_; i++) {
      if (evaluated_predicates.contains(i)) {
        continue;
      }
      order.push_back(i);
    }

    std::shuffle(order.begin(), order.end(), rng_);
    return environment_.Execute(index_order, order);
  }

  int SelectAction() {
    if (!priority_actions_.empty()) {
      int num_untried = priority_actions_.size();
      int action_idx = rng_() % num_untried;
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

  ScanSelectEnvironment& environment_;
  int created_in_;
  int num_predicates_;
  const std::vector<int>& index_predicates_;
  int current_idx_;
  int num_actions_;
  std::unique_ptr<PredicateOrderUCTNode> predicate_order_node_;
  std::vector<std::unique_ptr<IndexUCTNode>> child_nodes_;
  int num_visits_;
  std::vector<int> priority_actions_;
  std::vector<int> num_tries_per_action_;
  std::vector<double> acc_reward_per_action_;
  std::vector<int> predicate_per_action_;
  absl::btree_set<int> evaluated_predicates_;
  std::vector<int> unevaluated_predicates_;
  std::default_random_engine rng_;

  static constexpr double EXPLORATION_WEIGHT_ = 1E-5;

  friend class PredicateOrderUCTNode;
};

PredicateOrderUCTNode::PredicateOrderUCTNode(int round_ctr,
                                             IndexUCTNode& parent)
    : environment_(parent.environment_),
      created_in_(round_ctr),
      num_predicates_(parent.num_predicates_),
      num_actions_(num_predicates_ - parent.evaluated_predicates_.size()),
      child_nodes_(num_actions_),
      num_visits_(0),
      num_tries_per_action_(num_actions_, 0),
      acc_reward_per_action_(num_actions_, 0),
      predicate_per_action_(num_actions_),
      rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
  evaluated_predicates_.insert(parent.evaluated_predicates_.begin(),
                               parent.evaluated_predicates_.end());
  int action = 0;
  for (int i = 0; i < num_predicates_; i++) {
    if (!evaluated_predicates_.contains(i)) {
      unevaluated_predicates_.push_back(i);
      priority_actions_.push_back(action);
      predicate_per_action_[action++] = i;
    }
  }
}

class UctScanSelectAgent {
 public:
  UctScanSelectAgent(int num_predicates,
                     const std::vector<int32_t>& indexed_predicates,
                     ScanSelectEnvironment& environment)
      : environment_(environment),
        round_ctr_(0),
        num_predicates_(num_predicates),
        indexed_predicates_(indexed_predicates),
        should_forget_(FLAGS_scan_select_forget.Get()),
        next_forget_(10),
        root_(std::make_unique<IndexUCTNode>(
            round_ctr_, environment_, num_predicates_, indexed_predicates_)) {
    order_.reserve(num_predicates_);
    index_order_.reserve(num_predicates_);
  }

  void Act() {
    round_ctr_++;

    order_.clear();
    index_order_.clear();
    root_->Sample(round_ctr_, index_order_, order_);

    // Consider memory loss
    if (should_forget_ && round_ctr_ == next_forget_) {
      root_ = std::make_unique<IndexUCTNode>(
          round_ctr_, environment_, num_predicates_, indexed_predicates_);
      next_forget_ *= 10;
    }
  }

 private:
  ScanSelectEnvironment& environment_;
  int round_ctr_;
  int num_predicates_;
  const std::vector<int32_t>& indexed_predicates_;
  bool should_forget_;
  int64_t next_forget_;
  std::unique_ptr<IndexUCTNode> root_;
  std::vector<int> order_;
  std::vector<int> index_order_;
};

class ScanSelectState {
 public:
  ScanSelectState(int32_t cardinality)
      : cardinality_(cardinality), last_completed_tuple_(-1) {}

  bool IsComplete() { return last_completed_tuple_ == cardinality_; }

  int32_t GetLastCompletedTupleIdx() { return last_completed_tuple_; }

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

class PermutableScanSelectEnvironment : public ScanSelectEnvironment {
 public:
  PermutableScanSelectEnvironment(
      int32_t cardinality,
      const std::add_pointer<int32_t(int32_t, int32_t)>::type main_fn,
      const std::vector<std::add_pointer<bool()>::type> predicate_fn,
      PermutableScanSelectExecutionEngineFlags execution_engine)
      : cardinality_(cardinality),
        budget_per_episode_(FLAGS_scan_select_budget_per_episode.Get()),
        main_fn_(main_fn),
        predicate_fn_(predicate_fn),
        state_(cardinality),
        execution_engine_(execution_engine) {}

  bool IsComplete() { return state_.IsComplete(); }

  double Execute(const std::vector<int>& index_order,
                 const std::vector<int>& order) override {
    auto initial_last_completed_tuple = state_.GetLastCompletedTupleIdx();

    SetExecutionEngine(index_order, order);
    auto status =
        main_fn_(budget_per_episode_, initial_last_completed_tuple + 1);

    auto final_last_completed_tuple =
        ComputeLastCompletedTuple(*execution_engine_.idx, status);

    state_.Update(final_last_completed_tuple);

    return Reward(initial_last_completed_tuple, final_last_completed_tuple,
                  cardinality_);
  }

 private:
  void SetExecutionEngine(const std::vector<int>& index_order,
                          const std::vector<int>& order) {
    *execution_engine_.index_array_size = index_order.size();
    for (int i = 0; i < index_order.size(); i++) {
      execution_engine_.index_array[i] = index_order[i];
    }

    for (int i = 0; i < order.size(); i++) {
      execution_engine_.predicate_arr[i] = predicate_fn_[order[i]];
    }
  }

  int32_t cardinality_;
  int32_t budget_per_episode_;
  const std::add_pointer<int32_t(int32_t, int32_t)>::type main_fn_;
  const std::vector<std::add_pointer<bool()>::type> predicate_fn_;
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
*/

void ExecutePermutableSkinnerScanSelect(
    std::vector<int>* index_executable_predicates,
    std::add_pointer<int32_t(int32_t, int32_t)>::type main_fn,
    int32_t* index_array, int32_t* index_array_size,
    std::add_pointer<bool()>::type* predicate_arr, int32_t num_predicates,
    int32_t* idx) {
  int32_t cardinality = *idx;
  PermutableScanSelectExecutionEngineFlags execution_engine{
      .idx = idx,
      .index_array = index_array,
      .index_array_size = index_array_size,
      .predicate_arr = predicate_arr,
  };

  std::vector<std::add_pointer<bool()>::type> predicate_fn;
  for (int i = 0; i < num_predicates; i++) {
    predicate_fn.push_back(predicate_arr[i]);
  }

  PermutableScanSelectEnvironment environment(cardinality, main_fn,
                                              predicate_fn, execution_engine);

  UctScanSelectAgent agent(num_predicates, *index_executable_predicates,
                           environment);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

std::add_pointer<bool()>::type GetPredicateFn(
    std::add_pointer<bool()>::type* preds, int idx) {
  return preds[idx];
}

}  // namespace kush::runtime