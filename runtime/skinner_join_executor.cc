#include "runtime/skinner_join_executor.h"

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

#include "compile/translators/recompiling_join_translator.h"

ABSL_FLAG(int32_t, budget_per_episode, 10000, "Budget per episode");
ABSL_FLAG(bool, forget, false, "Forget learned info periodically.");
ABSL_FLAG(int64_t, join_seed, -1, "Join seed.");

namespace kush::runtime {

int64_t GetJoinSeed() {
  if (FLAGS_join_seed.Get() > 0) {
    return FLAGS_join_seed.Get();
  }

  return std::chrono::system_clock::now().time_since_epoch().count();
}

class JoinState {
 private:
  class ProgressTree {
   public:
    ProgressTree(int num_tables) : child_nodes(num_tables) {}
    int32_t last_completed_tuple;
    std::vector<std::unique_ptr<ProgressTree>> child_nodes;
  };

  bool IsJoinCompleted(const std::vector<int32_t>& last_completed_tuple,
                       const std::vector<int32_t>& cardinalities) {
    for (int i = 0; i < last_completed_tuple.size(); i++) {
      if (last_completed_tuple[i] != cardinalities[i] - 1) {
        return false;
      }
    }
    return true;
  }

 public:
  JoinState(const std::vector<int32_t>& cardinalities)
      : finished_(false),
        cardinalities_(cardinalities),
        root_(cardinalities.size()),
        offset_(cardinalities_.size(), -1) {}

  bool IsComplete() { return finished_; }

  const std::vector<int32_t>& GetOffset() { return offset_; }

  std::optional<std::vector<int32_t>> GetLastCompletedTupleIdx(
      const std::vector<int>& order) {
    std::vector<int32_t> last_completed_tuple_idx(order.size());
    ProgressTree* prev = &root_;
    for (int i = 0; i < order.size(); i++) {
      int table = order[i];
      std::unique_ptr<ProgressTree>& curr = prev->child_nodes[table];

      if (curr == nullptr) {
        // first time executing this order at all
        if (prev == &root_) {
          return std::nullopt;
        }

        // set all remaining tables to value 0
        for (int j = i; j < order.size(); j++) {
          int table = order[j];
          last_completed_tuple_idx[table] = 0;
        }

        // attempt to decrement
        for (int i = order.size() - 1; i >= 0; i--) {
          int table = order[i];

          if (last_completed_tuple_idx[table] == 0) {
            if (i == 0) {
              // we've decremented back to nothing
              return std::nullopt;
            }

            last_completed_tuple_idx[table] = cardinalities_[table] - 1;
          } else {
            last_completed_tuple_idx[table]--;
            break;
          }
        }

        return last_completed_tuple_idx;
      }

      last_completed_tuple_idx[table] = curr->last_completed_tuple;
      prev = curr.get();
    }
    return last_completed_tuple_idx;
  }

  void Update(const std::vector<int>& order,
              const std::vector<int32_t>& last_completed_tuple_idx) {
    if (finished_) {
      throw std::runtime_error("Already done executing join.");
    }

    if (IsJoinCompleted(last_completed_tuple_idx, cardinalities_)) {
      finished_ = true;
      return;
    }

    // Update offset
    {
      int first_table = order[0];
      int32_t last_finished_tuple = last_completed_tuple_idx[first_table];

      bool completed_last_finished_tuple = true;
      for (int i = 1; i < order.size(); i++) {
        auto table_idx = order[i];
        if (last_completed_tuple_idx[table_idx] !=
            cardinalities_[table_idx] - 1) {
          completed_last_finished_tuple = false;
          break;
        }
      }
      if (!completed_last_finished_tuple) {
        last_finished_tuple--;
      }

      offset_[first_table] =
          std::max(last_finished_tuple, offset_[first_table]);
    }

    // Update progress tree
    ProgressTree* prev = &root_;
    for (int i = 0; i < order.size(); i++) {
      int32_t table = order[i];
      int32_t last_completed_tuple = last_completed_tuple_idx[table];
      std::unique_ptr<ProgressTree>& curr = prev->child_nodes[table];

      if (curr == nullptr) {
        for (int j = i; j < order.size(); j++) {
          int32_t table = order[j];
          int32_t last_completed_tuple = last_completed_tuple_idx[table];
          std::unique_ptr<ProgressTree>& curr = prev->child_nodes[table];
          curr = std::make_unique<ProgressTree>(order.size());
          curr->last_completed_tuple = last_completed_tuple;
          prev = curr.get();
        }
        return;
      }

      if (last_completed_tuple < curr->last_completed_tuple) {
        throw std::runtime_error("Negative progress was made");
      }

      if (last_completed_tuple > curr->last_completed_tuple) {
        // fast forwarding this node
        curr->last_completed_tuple = last_completed_tuple;

        // delete all children since we've never executed them with the current
        // last_completed_tuple
        for (auto& x : curr->child_nodes) {
          x.reset();
        }

        // set join order
        prev = curr.get();
        for (int j = i + 1; j < order.size(); j++) {
          int32_t table = order[j];
          int32_t last_completed_tuple = last_completed_tuple_idx[table];
          std::unique_ptr<ProgressTree>& curr = prev->child_nodes[table];
          curr = std::make_unique<ProgressTree>(order.size());
          curr->last_completed_tuple = last_completed_tuple;
          prev = curr.get();
        }
        return;
      }

      prev = curr.get();
    }
  }

 private:
  bool finished_;
  const std::vector<int32_t> cardinalities_;
  ProgressTree root_;
  std::vector<int32_t> offset_;
};

class JoinEnvironment {
 public:
  virtual bool IsConnected(const absl::btree_set<int>& joined_tables,
                           int table) = 0;
  virtual bool IsComplete() = 0;
  virtual double Execute(const std::vector<int>& order) = 0;

  std::vector<int32_t> ComputeLastCompletedTuple(
      const std::vector<int>& order, const std::vector<int32_t>& cardinalities,
      int status, int table_ctr, int* idx_arr) {
    std::vector<int32_t> last_completed_tuple(order.size(), 0);
    auto should_decrement = status == -2;

    int table_ctr_idx = -1;
    for (int i = 0; i < order.size(); i++) {
      int table = order[i];
      last_completed_tuple[table] = idx_arr[table];
      if (table_ctr == table) {
        table_ctr_idx = i;
      }
    }

    if (!should_decrement) {
      // every table after table_ctr is just cardinality - 1
      for (int i = table_ctr_idx + 1; i < order.size(); i++) {
        int table = order[i];
        last_completed_tuple[table] = cardinalities[table] - 1;
      }
      return last_completed_tuple;
    }

    // fill in every table after table_ctr with 0
    for (int i = table_ctr_idx + 1; i < order.size(); i++) {
      int table = order[i];
      last_completed_tuple[table] = 0;
    }

    // decrement last_completed_tuple by 1
    for (int i = order.size() - 1; i >= 0; i--) {
      int table = order[i];

      if (last_completed_tuple[table] == 0) {
        last_completed_tuple[table] = cardinalities[table] - 1;
      } else {
        last_completed_tuple[table]--;
        break;
      }
    }

    return last_completed_tuple;
  }

  double Reward(const std::vector<int32_t>& order,
                const std::vector<int32_t>& offset,
                const std::vector<int32_t>& initial_last_completed_tuple,
                const std::vector<int32_t>& final_last_completed_tuple,
                const std::vector<int32_t>& cardinalities,
                int32_t num_result_tuples, int32_t budget_per_episode) {
    double progress = 0;
    double weight = 1;
    for (int i = 0; i < order.size(); i++) {
      int table = order[i];
      int remaining_card = cardinalities[table] - offset[table];
      assert(remaining_card > 0);
      weight *= 1.0 / remaining_card;

      int start = std::max(offset[table], initial_last_completed_tuple[table]);
      int end = std::max(offset[table], final_last_completed_tuple[table]);

      progress += (end - start) * weight;
    }

    return 0.5 * progress +
           0.5 * num_result_tuples / (double)budget_per_episode;
  }
};

struct PermutableExecutionEngineFlags {
  std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr;
  int8_t* flag_arr;
  int32_t* progress_arr;
  int32_t* table_ctr;
  int32_t* idx_arr;
  int32_t* num_result_tuples;
  int32_t* offset_arr;
};

class PermutableJoinEnvironment : public JoinEnvironment {
 public:
  PermutableJoinEnvironment(
      int32_t num_preds, int32_t num_flags,
      const absl::flat_hash_map<std::pair<int, int>, int>* pred_table_to_flag,
      const absl::flat_hash_set<std::pair<int, int>>* table_connections,
      const std::vector<int32_t>& cardinalities,
      const std::vector<std::add_pointer<int(int, int8_t)>::type>&
          table_functions,
      const std::add_pointer<int32_t(int32_t, int8_t)>::type
          valid_tuple_handler,
      PermutableExecutionEngineFlags execution_engine)
      : num_preds_(num_preds),
        num_flags_(num_flags),
        budget_per_episode_(FLAGS_budget_per_episode.Get()),
        pred_table_to_flag_(pred_table_to_flag),
        table_connections_(table_connections),
        cardinalities_(cardinalities),
        table_functions_(table_functions),
        valid_tuple_handler_(valid_tuple_handler),
        state_(cardinalities_),
        execution_engine_(execution_engine) {}

  bool IsConnected(const absl::btree_set<int>& joined_tables,
                   int table) override {
    for (int table1 : joined_tables) {
      if (table_connections_->contains({table1, table})) {
        return true;
      }
    }

    return false;
  }

  bool IsComplete() override { return state_.IsComplete(); }

  double Execute(const std::vector<int>& order) override {
    auto initial_last_completed_tuple = state_.GetLastCompletedTupleIdx(order);
    const auto& offset = state_.GetOffset();

    SetJoinOrder(order);
    TogglePredicateFlags(order);
    SetResumeProgress(order,
                      initial_last_completed_tuple.value_or(
                          std::vector<int32_t>(order.size(), -1)),
                      offset);

    auto status = table_functions_[order[0]](
        budget_per_episode_, initial_last_completed_tuple.has_value() ? 1 : 0);

    auto final_last_completed_tuple = ComputeLastCompletedTuple(
        order, cardinalities_, status, *execution_engine_.table_ctr,
        execution_engine_.idx_arr);

    state_.Update(order, final_last_completed_tuple);

    return Reward(order, offset,
                  initial_last_completed_tuple.value_or(
                      std::vector<int32_t>(order.size(), -1)),
                  final_last_completed_tuple, cardinalities_,
                  *execution_engine_.num_result_tuples, budget_per_episode_);
  }

 private:
  void SetJoinOrder(const std::vector<int>& order) {
    for (int i = 0; i < order.size() - 1; i++) {
      // set the ith handler to i+1 function
      int current_table = order[i];
      int next_table = order[i + 1];

      execution_engine_.join_handler_fn_arr[current_table] =
          table_functions_[next_table];
    }

    // set last handler to be the valid tuple handler
    execution_engine_.join_handler_fn_arr[order[order.size() - 1]] =
        valid_tuple_handler_;
  }

  void SetResumeProgress(const std::vector<int>& order,
                         const std::vector<int32_t>& last_completed_tuple,
                         const std::vector<int32_t>& offset) {
    for (int table = 0; table < last_completed_tuple.size(); table++) {
      execution_engine_.progress_arr[table] =
          std::max(offset[table], last_completed_tuple[table]);
      execution_engine_.offset_arr[table] = offset[table];
    }
    *execution_engine_.table_ctr = order[0];
    *execution_engine_.num_result_tuples = 0;
  }

  void TogglePredicateFlags(const std::vector<int>& order) {
    // Reset all flags to 0
    for (int i = 0; i < num_flags_; i++) {
      execution_engine_.flag_arr[i] = 0;
    }

    // Set all predicate flags
    absl::flat_hash_set<int> available_tables;
    absl::flat_hash_set<int> executed_preds;
    for (int order_idx = 0; order_idx < order.size(); order_idx++) {
      auto table = order[order_idx];
      available_tables.insert(table);

      // all tables in available_tables have been joined
      // execute any non-executed general predicate
      for (int pred = 0; pred < num_preds_; pred++) {
        if (executed_preds.contains(pred)) {
          continue;
        }

        bool all_tables_available = true;
        for (int t = 0; t < order.size(); t++) {
          if (pred_table_to_flag_->contains({pred, t})) {
            all_tables_available =
                all_tables_available && available_tables.contains(t);
          }
        }
        if (!all_tables_available) {
          continue;
        }
        executed_preds.insert(pred);

        auto flag = pred_table_to_flag_->at({pred, table});
        execution_engine_.flag_arr[flag] = 1;
      }
    }
  }

  const int num_preds_;
  const int num_flags_;
  const int32_t budget_per_episode_;
  const absl::flat_hash_map<std::pair<int, int>, int>* pred_table_to_flag_;

  const absl::flat_hash_set<std::pair<int, int>>* table_connections_;
  const std::vector<int32_t> cardinalities_;
  const std::vector<std::add_pointer<int(int, int8_t)>::type> table_functions_;
  const std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_;
  JoinState state_;
  PermutableExecutionEngineFlags execution_engine_;
};

struct RecompilationExecutionEngineFlags {
  int32_t* progress_arr;
  int32_t* table_ctr;
  int32_t* idx_arr;
  int32_t* offset_arr;
  int32_t* num_result_tuples;
};

class RecompilationJoinEnvironment : public JoinEnvironment {
 public:
  RecompilationJoinEnvironment(
      compile::RecompilingJoinTranslator* codegen, void** materialized_buffers,
      void** materialized_indexes, void* tuple_idx_table,
      const absl::flat_hash_set<std::pair<int, int>>* table_connections,
      const std::vector<int32_t>& cardinalities,
      RecompilationExecutionEngineFlags execution_engine,
      std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler)
      : codegen_(codegen),
        materialized_buffers_(materialized_buffers),
        materialized_indexes_(materialized_indexes),
        tuple_idx_table_(tuple_idx_table),
        budget_per_episode_(FLAGS_budget_per_episode.Get()),
        table_connections_(table_connections),
        cardinalities_(cardinalities),
        state_(cardinalities_),
        execution_engine_(execution_engine),
        valid_tuple_handler_(valid_tuple_handler) {}

  bool IsConnected(const absl::btree_set<int>& joined_tables,
                   int table) override {
    for (int table1 : joined_tables) {
      if (table_connections_->contains({table1, table})) {
        return true;
      }
    }

    return false;
  }

  bool IsComplete() override { return state_.IsComplete(); }

  double Execute(const std::vector<int>& order) override {
    auto initial_last_completed_tuple = state_.GetLastCompletedTupleIdx(order);
    const auto& offset = state_.GetOffset();

    SetResumeProgress(order,
                      initial_last_completed_tuple.value_or(
                          std::vector<int32_t>(order.size(), -1)),
                      offset);
    /*
        std::cerr << "Order:";
        for (int i = 0; i < order.size(); i++) {
          std::cerr << ' ' << order[i];
        }
        std::cerr << std::endl;
        std::cerr << "Progress:";
        for (int i = 0; i < order.size(); i++) {
          std::cerr << ' ' << execution_engine_.progress_arr[i];
        }
        std::cerr << std::endl;
        std::cerr << "Cardinalities:";
        for (int i = 0; i < order.size(); i++) {
          std::cerr << ' ' << cardinalities_[i];
        }
        std::cerr << std::endl;
    */
    auto execute_fn = codegen_->CompileJoinOrder(
        order, materialized_buffers_, materialized_indexes_, tuple_idx_table_,
        execution_engine_.progress_arr, execution_engine_.table_ctr,
        execution_engine_.idx_arr, execution_engine_.offset_arr,
        valid_tuple_handler_);

    auto status = execute_fn(budget_per_episode_,
                             initial_last_completed_tuple.has_value());
    /*
        std::cerr << "Status: " << status << std::endl;
        std::cerr << "Table CTR: " << *execution_engine_.table_ctr << std::endl;
        std::cerr << "IDX:";
        for (int i = 0; i < order.size(); i++) {
          std::cerr << ' ' << execution_engine_.idx_arr[i];
        }
        std::cerr << std::endl;
    */
    auto final_last_completed_tuple = ComputeLastCompletedTuple(
        order, cardinalities_, status, *execution_engine_.table_ctr,
        execution_engine_.idx_arr);
    /*
        std::cerr << "Final Completed Tuple:";
        for (int i = 0; i < order.size(); i++) {
          std::cerr << ' ' << final_last_completed_tuple[i];
        }
        std::cerr << std::endl;
    */
    state_.Update(order, final_last_completed_tuple);

    auto reward =
        Reward(order, offset,
               initial_last_completed_tuple.value_or(
                   std::vector<int32_t>(order.size(), -1)),
               final_last_completed_tuple, cardinalities_,
               *execution_engine_.num_result_tuples, budget_per_episode_);
    //    std::cerr << "Reward: " << reward << std::endl;
    return reward;
  }

 private:
  void SetResumeProgress(const std::vector<int>& order,
                         const std::vector<int32_t>& last_completed_tuple,
                         const std::vector<int32_t>& offset) {
    for (int table = 0; table < last_completed_tuple.size(); table++) {
      execution_engine_.progress_arr[table] =
          std::max(offset[table], last_completed_tuple[table]);
      execution_engine_.offset_arr[table] = offset[table];
    }
    *execution_engine_.table_ctr = order[0];
    *execution_engine_.num_result_tuples = 0;
  }

  compile::RecompilingJoinTranslator* codegen_;
  void** materialized_buffers_;
  void** materialized_indexes_;
  void* tuple_idx_table_;
  const int32_t budget_per_episode_;
  const absl::flat_hash_set<std::pair<int, int>>* table_connections_;
  const std::vector<int32_t> cardinalities_;
  JoinState state_;
  RecompilationExecutionEngineFlags execution_engine_;
  std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_;
};

class UctNode {
 public:
  UctNode(int round_ctr, int num_tables, JoinEnvironment& environment,
          const std::vector<int>& already_joined)
      : environment_(environment),
        created_in_(round_ctr),
        tree_level_(0),
        num_tables_(num_tables),
        num_actions_(num_tables - already_joined.size()),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        table_per_action_(num_actions_),
        rng_(GetJoinSeed()) {
    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
      recommended_actions_.insert(i);
    }

    for (int x : already_joined) {
      joined_tables_.insert(x);
    }

    int action = 0;
    for (int i = 0; i < num_tables_; i++) {
      if (!joined_tables_.contains(i)) {
        unjoined_tables_.push_back(i);
        table_per_action_[action++] = i;
      }
    }
  }

  UctNode(int round_ctr, UctNode& parent, int joined_table)
      : environment_(parent.environment_),
        created_in_(round_ctr),
        tree_level_(parent.tree_level_ + 1),
        num_tables_(parent.num_tables_),
        num_actions_(parent.num_actions_ - 1),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        joined_tables_(parent.joined_tables_),
        unjoined_tables_(parent.unjoined_tables_),
        table_per_action_(num_actions_),
        rng_(GetJoinSeed()) {
    joined_tables_.insert(joined_table);

    auto it = std::find(unjoined_tables_.begin(), unjoined_tables_.end(),
                        joined_table);
    unjoined_tables_.erase(it);

    int action = 0;
    for (int table : unjoined_tables_) {
      table_per_action_[action++] = table;
    }

    if (USE_HEURISTIC_) {
      for (int i = 0; i < num_actions_; i++) {
        int table = table_per_action_[i];
        if (environment_.IsConnected(joined_tables_, table)) {
          recommended_actions_.insert(i);
        }
      }

      if (recommended_actions_.empty()) {
        for (int i = 0; i < num_actions_; i++) {
          recommended_actions_.insert(i);
        }
      }
    }

    for (int i = 0; i < num_actions_; i++) {
      if (!USE_HEURISTIC_ ||
          recommended_actions_.find(i) != recommended_actions_.end()) {
        priority_actions_.push_back(i);
      }
    }
  }

  double Sample(int round_ctr, std::vector<int>& order) {
    if (num_actions_ == 0) {
      return environment_.Execute(order);
    }

    int action = SelectAction();
    int table = table_per_action_[action];
    order[joined_tables_.size()] = table;

    bool can_expand = created_in_ != round_ctr;
    if (can_expand && child_nodes_[action] == nullptr) {
      child_nodes_[action] = std::make_unique<UctNode>(round_ctr, *this, table);
    }

    UctNode* child = child_nodes_[action].get();

    double reward =
        child != nullptr ? child->Sample(round_ctr, order) : Playout(order);

    UpdateStatistics(action, reward);
    return reward;
  }

 private:
  double Playout(std::vector<int>& order) {
    int last_table = order[joined_tables_.size()];

    absl::btree_set<int> current_joined;
    current_joined.insert(joined_tables_.begin(), joined_tables_.end());
    current_joined.insert(last_table);

    std::vector<int> current_unjoined;
    for (int i = 0; i < num_tables_; i++) {
      if (!joined_tables_.contains(i) && i != last_table) {
        current_unjoined.push_back(i);
      }
    }

    if (USE_HEURISTIC_) {
      std::shuffle(current_unjoined.begin(), current_unjoined.end(), rng_);
      for (int pos = joined_tables_.size() + 1; pos < num_tables_; pos++) {
        bool found_table = false;
        for (int table : current_unjoined) {
          if (current_joined.find(table) == current_joined.end() &&
              environment_.IsConnected(current_joined, table)) {
            order[pos] = table;
            current_joined.insert(table);
            found_table = true;
            break;
          }
        }
        if (!found_table) {
          for (int table : current_unjoined) {
            if (current_joined.find(table) == current_joined.end()) {
              order[pos] = table;
              current_joined.insert(table);
              break;
            }
          }
        }
      }
    } else {
      std::shuffle(current_unjoined.begin(), current_unjoined.end(), rng_);
      for (int i = 0; i < current_unjoined.size(); i++) {
        int table = current_unjoined[i];
        order[(joined_tables_.size() + 1) + i] = table;
      }
    }

    return environment_.Execute(order);
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

      if (USE_HEURISTIC_ && recommended_actions_.empty()) {
        throw std::runtime_error("no recommended actions with heuristic.");
      }

      if (USE_HEURISTIC_ &&
          recommended_actions_.find(action) == recommended_actions_.end()) {
        continue;
      }

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
  JoinEnvironment& environment_;
  int created_in_;
  int tree_level_;
  int num_tables_;
  int num_actions_;
  std::vector<int> priority_actions_;
  std::vector<std::unique_ptr<UctNode>> child_nodes_;
  int num_visits_;
  std::vector<int> num_tries_per_action_;
  std::vector<double> acc_reward_per_action_;
  absl::btree_set<int> joined_tables_;
  absl::btree_set<int> recommended_actions_;
  std::vector<int> unjoined_tables_;
  std::vector<int> table_per_action_;
  std::default_random_engine rng_;

  static constexpr double EXPLORATION_WEIGHT_ = 1E-5;
  static constexpr bool USE_HEURISTIC_ = true;
};

class UctJoinAgent {
 public:
  UctJoinAgent(int num_tables, JoinEnvironment& environment,
               const std::vector<int>* joined)
      : environment_(environment),
        round_ctr_(0),
        num_tables_(num_tables),
        should_forget_(FLAGS_forget.Get()),
        next_forget_(10),
        joined_(joined),
        root_(std::make_unique<UctNode>(round_ctr_, num_tables, environment,
                                        *joined_)),
        order_(num_tables_) {
    int i = 0;
    for (int t : *joined_) {
      order_[i++] = t;
    }
  }

  void Act() {
    round_ctr_++;
    root_->Sample(round_ctr_, order_);

    // Consider memory loss
    if (should_forget_ && round_ctr_ == next_forget_) {
      root_ = std::make_unique<UctNode>(round_ctr_, num_tables_, environment_,
                                        *joined_);
      next_forget_ *= 10;
    }
  }

 private:
  JoinEnvironment& environment_;
  int round_ctr_;
  int num_tables_;
  bool should_forget_;
  int64_t next_forget_;
  const std::vector<int>* joined_;
  std::unique_ptr<UctNode> root_;
  std::vector<int> order_;
};

std::vector<std::add_pointer<int(int, int8_t)>::type> ReconstructTableFunctions(
    int32_t num_tables,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr) {
  std::vector<std::add_pointer<int(int, int8_t)>::type> table_functions;
  for (int i = 0; i < num_tables; i++) {
    table_functions.push_back(join_handler_fn_arr[i]);
  }
  return table_functions;
}

std::vector<int32_t> ReconstructCardinalities(int32_t num_tables,
                                              int32_t* idx_arr) {
  std::vector<int32_t> cardinalities;
  for (int i = 0; i < num_tables; i++) {
    cardinalities.push_back(idx_arr[i]);
  }
  return cardinalities;
}

void ExecutePermutableSkinnerJoin(
    int32_t num_tables, int32_t num_preds,
    const absl::flat_hash_map<std::pair<int, int>, int>* pred_table_to_flag,
    const absl::flat_hash_set<std::pair<int, int>>* table_connections,
    const std::vector<int>* prefix_order,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    int32_t num_flags, int8_t* flag_arr, int32_t* progress_arr,
    int32_t* table_ctr, int32_t* idx_arr, int32_t* num_result_tuples,
    int32_t* offset_arr) {
  auto table_functions =
      ReconstructTableFunctions(num_tables, join_handler_fn_arr);
  auto cardinalities = ReconstructCardinalities(num_tables, idx_arr);
  for (int x : cardinalities) {
    // one of the inputs is empty
    if (x == 0) {
      return;
    }
  }

  PermutableExecutionEngineFlags execution_engine{
      .join_handler_fn_arr = join_handler_fn_arr,
      .flag_arr = flag_arr,
      .progress_arr = progress_arr,
      .table_ctr = table_ctr,
      .idx_arr = idx_arr,
      .num_result_tuples = num_result_tuples,
      .offset_arr = offset_arr,
  };

  PermutableJoinEnvironment environment(
      num_preds, num_flags, pred_table_to_flag, table_connections,
      cardinalities, table_functions, valid_tuple_handler, execution_engine);

  UctJoinAgent agent(num_tables, environment, prefix_order);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

void ExecuteRecompilingSkinnerJoin(
    int32_t num_tables, int32_t* cardinality_arr,
    const absl::flat_hash_set<std::pair<int, int>>* table_connections,
    const std::vector<int>* prefix_order,
    compile::RecompilingJoinTranslator* codegen, void** materialized_buffers,
    void** materialized_indexes, void* tuple_idx_table, int32_t* idx_arr,
    int32_t* num_result_tuples,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler) {
  auto cardinalities = ReconstructCardinalities(num_tables, cardinality_arr);
  for (int x : cardinalities) {
    // one of the inputs is empty
    if (x == 0) {
      return;
    }
  }

  auto progress_arr = new int32_t[num_tables];
  int32_t table_ctr = 0;

  auto offset_arr = new int32_t[num_tables];
  for (int i = 0; i < num_tables; i++) {
    idx_arr[i] = 0;
    progress_arr[i] = -1;
    offset_arr[i] = -1;
  }

  RecompilationExecutionEngineFlags execution_engine{
      .progress_arr = progress_arr,
      .table_ctr = &table_ctr,
      .idx_arr = idx_arr,
      .offset_arr = offset_arr,
      .num_result_tuples = num_result_tuples,
  };

  RecompilationJoinEnvironment environment(
      codegen, materialized_buffers, materialized_indexes, tuple_idx_table,
      table_connections, cardinalities, execution_engine, valid_tuple_handler);

  UctJoinAgent agent(num_tables, environment, prefix_order);

  while (!environment.IsComplete()) {
    agent.Act();
  }

  delete[] progress_arr;
  delete[] offset_arr;
}

}  // namespace kush::runtime