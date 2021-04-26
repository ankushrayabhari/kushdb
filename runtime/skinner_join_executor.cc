#include "runtime/skinner_join_executor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kush::runtime {

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
        root_(cardinalities.size()) {}

  bool IsComplete() { return finished_; }

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
  std::vector<int32_t> cardinalities_;
  ProgressTree root_;
};

struct ExecutionEngineFlags {
  std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr;
  int8_t* flag_arr;
  int32_t* progress_arr;
  int32_t* table_ctr;
  int32_t* idx_arr;
  int32_t* last_table;
  int32_t* num_result_tuples;
};

class JoinEnvironment {
 public:
  JoinEnvironment(
      int num_predicates,
      const std::vector<std::unordered_map<int, int>>&
          table_predicate_to_flag_idx,
      const std::vector<std::unordered_set<int>>& tables_per_predicate,
      const std::vector<int32_t>& cardinalities,
      const std::vector<std::add_pointer<int(int, int8_t)>::type>&
          table_functions,
      const std::add_pointer<int32_t(int32_t, int8_t)>::type
          valid_tuple_handler,
      ExecutionEngineFlags execution_engine)
      : num_predicates_(num_predicates),
        budget_per_episode_(10000),
        table_predicate_to_flag_idx_(table_predicate_to_flag_idx),
        tables_per_predicate_(tables_per_predicate),
        cardinalities_(cardinalities),
        table_functions_(table_functions),
        valid_tuple_handler_(valid_tuple_handler),
        state_(cardinalities_),
        execution_engine_(execution_engine) {}

  bool IsConnected(const std::set<int>& joined_tables, int table) {
    for (int i = 0; i < num_predicates_; i++) {
      const auto& tables = tables_per_predicate_[i];

      if (tables.find(table) == tables.end()) {
        continue;
      }

      for (int x : tables) {
        if (joined_tables.find(x) != joined_tables.end()) {
          return true;
        }
      }
    }

    return false;
  }

  bool IsComplete() { return state_.IsComplete(); }

  double Execute(const std::vector<int>& order) {
    auto initial_last_completed_tuple = state_.GetLastCompletedTupleIdx(order);

    SetJoinOrder(order);
    TogglePredicateFlags(order);
    SetResumeProgress(order, initial_last_completed_tuple.value_or(
                                 std::vector<int32_t>(order.size(), -1)));

    auto status = table_functions_[order[0]](
        budget_per_episode_, initial_last_completed_tuple.has_value() ? 1 : 0);

    auto final_last_completed_tuple =
        ComputeLastCompletedTuple(order, cardinalities_, status);

    state_.Update(order, final_last_completed_tuple);

    return Reward(initial_last_completed_tuple.value_or(
                      std::vector<int32_t>(order.size(), -1)),
                  final_last_completed_tuple);
  }

 private:
  double Reward(const std::vector<int32_t>& initial_last_completed_tuple,
                const std::vector<int32_t>& final_last_completed_tuple) {
    return 0.5 * (*execution_engine_.num_result_tuples) /
           (double)budget_per_episode_;
  }

  void SetJoinOrder(const std::vector<int>& order) {
    *execution_engine_.last_table = order.back();

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
                         const std::vector<int32_t>& last_completed_tuple) {
    for (int table = 0; table < last_completed_tuple.size(); table++) {
      execution_engine_.progress_arr[table] = last_completed_tuple[table];
    }
    *execution_engine_.table_ctr = order[0];
    *execution_engine_.num_result_tuples = 0;
  }

  void TogglePredicateFlags(const std::vector<int>& order) {
    // Reset all predicate flags to 0
    for (const auto& predicate_flag : table_predicate_to_flag_idx_) {
      for (const auto& [predicate, flag_idx] : predicate_flag) {
        execution_engine_.flag_arr[flag_idx] = 0;
      }
    }

    // Set all predicate flags
    std::unordered_set<int> available_tables;
    std::unordered_set<int> executed_predicates;
    for (int table : order) {
      available_tables.insert(table);

      // all tables in available_tables have been joined
      // execute any non-executed predicate
      for (int predicate_idx = 0; predicate_idx < num_predicates_;
           predicate_idx++) {
        if (executed_predicates.find(predicate_idx) !=
            executed_predicates.end())
          continue;

        bool all_tables_available = true;
        for (int t : tables_per_predicate_[predicate_idx]) {
          all_tables_available =
              all_tables_available &&
              available_tables.find(t) != available_tables.end();
        }
        if (!all_tables_available) continue;

        executed_predicates.insert(predicate_idx);
        execution_engine_
            .flag_arr[table_predicate_to_flag_idx_[table].at(predicate_idx)] =
            1;
      }
    }
  }

  std::vector<int32_t> ComputeLastCompletedTuple(
      const std::vector<int>& order, const std::vector<int32_t>& cardinalities,
      int status) {
    std::vector<int32_t> last_completed_tuple(order.size(), 0);
    auto should_decrement = status == -2;
    auto table_ctr = *execution_engine_.table_ctr;

    int table_ctr_idx = -1;
    for (int i = 0; i < order.size(); i++) {
      int table = order[i];
      last_completed_tuple[table] = execution_engine_.idx_arr[table];
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

  const int num_predicates_;
  const int32_t budget_per_episode_;
  const std::vector<std::unordered_map<int, int>> table_predicate_to_flag_idx_;
  const std::vector<std::unordered_set<int>> tables_per_predicate_;
  const std::vector<int32_t> cardinalities_;
  const std::vector<std::add_pointer<int(int, int8_t)>::type> table_functions_;
  const std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler_;
  JoinState state_;
  ExecutionEngineFlags execution_engine_;
};

class UctNode {
 public:
  UctNode(int round_ctr, int num_tables, JoinEnvironment& environment)
      : environment_(environment),
        created_in_(round_ctr),
        tree_level_(0),
        num_tables_(num_tables),
        num_actions_(num_tables),
        child_nodes_(num_actions_),
        num_visits_(0),
        num_tries_per_action_(num_actions_, 0),
        acc_reward_per_action_(num_actions_, 0),
        table_per_action_(num_actions_),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
      recommended_actions_.insert(i);
    }

    for (int i = 0; i < num_tables_; i++) {
      unjoined_tables_.push_back(i);
      table_per_action_[i] = i;
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
        rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
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
    order[tree_level_] = table;

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
    int last_table = order[tree_level_];

    if (USE_HEURISTIC_) {
      std::set<int> newly_joined;
      newly_joined.insert(joined_tables_.begin(), joined_tables_.end());
      newly_joined.insert(last_table);

      std::vector<int> unjoined_tables_shuffled(unjoined_tables_.begin(),
                                                unjoined_tables_.end());
      std::shuffle(unjoined_tables_shuffled.begin(),
                   unjoined_tables_shuffled.end(), rng_);
      for (int pos = tree_level_ + 1; pos < num_tables_; pos++) {
        bool found_table = false;
        for (int table : unjoined_tables_shuffled) {
          if (newly_joined.find(table) == newly_joined.end() &&
              environment_.IsConnected(newly_joined, table)) {
            order[pos] = table;
            newly_joined.insert(table);
            found_table = true;
            break;
          }
        }
        if (!found_table) {
          for (int table : unjoined_tables_shuffled) {
            if (newly_joined.find(table) == newly_joined.end()) {
              order[pos] = table;
              newly_joined.insert(table);
              break;
            }
          }
        }
      }
    } else {
      std::shuffle(unjoined_tables_.begin(), unjoined_tables_.end(), rng_);

      auto it = unjoined_tables_.begin();
      for (int level = tree_level_ + 1; level < num_tables_; ++level) {
        int table = *it++;

        if (table == last_table) {
          table = *it++;
        }

        order[level] = table;
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

    return best_action;
  }

  void UpdateStatistics(int action, double reward) {
    num_visits_++;
    num_tries_per_action_[action]++;
    acc_reward_per_action_[action] += reward;
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
  std::set<int> joined_tables_;
  std::set<int> recommended_actions_;
  std::vector<int> unjoined_tables_;
  std::vector<int> table_per_action_;
  std::default_random_engine rng_;

  static constexpr double EXPLORATION_WEIGHT_ = 1E-5;
  static constexpr bool USE_HEURISTIC_ = true;
};

class UctJoinAgent {
 public:
  UctJoinAgent(int num_tables, JoinEnvironment& environment)
      : round_ctr_(0),
        num_tables_(num_tables),
        root_(round_ctr_, num_tables, environment) {}

  void Act() {
    round_ctr_++;
    std::vector<int> order(num_tables_);
    root_.Sample(round_ctr_, order);
  }

 private:
  int round_ctr_;
  int num_tables_;
  UctNode root_;
};

std::vector<std::unordered_map<int, int>> ReconstructTablePredicateToFlagIdx(
    int32_t num_tables, int32_t table_predicate_to_flag_idx_len,
    int32_t* table_predicate_to_flag_idx_arr) {
  std::vector<std::unordered_map<int, int>> table_predicate_to_flag_idx(
      num_tables);
  for (int i = 0; i < table_predicate_to_flag_idx_len; i += 3) {
    int table = table_predicate_to_flag_idx_arr[i];
    int predicate = table_predicate_to_flag_idx_arr[i + 1];
    int flag_idx = table_predicate_to_flag_idx_arr[i + 2];
    table_predicate_to_flag_idx[table][predicate] = flag_idx;
  }
  return table_predicate_to_flag_idx;
}

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

std::vector<std::unordered_set<int>> ReconstructTablesPerPredicate(
    int32_t num_predicates, int32_t* tables_per_predicate_arr) {
  std::vector<std::unordered_set<int>> tables_per_predicate(num_predicates);
  std::vector<int> num_tables_per_predicate(num_predicates);
  for (int i = 0; i < num_predicates; i++) {
    num_tables_per_predicate[i] = tables_per_predicate_arr[i];
  }
  int last_idx = num_predicates;
  for (int i = 0; i < num_predicates; i++) {
    for (int j = 0; j < num_tables_per_predicate[i]; j++) {
      tables_per_predicate[i].insert(tables_per_predicate_arr[last_idx++]);
    }
  }
  return tables_per_predicate;
}

void ExecuteSkinnerJoin(
    int32_t num_tables, int32_t num_predicates,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    int32_t table_predicate_to_flag_idx_len,
    int32_t* table_predicate_to_flag_idx_arr, int32_t* tables_per_predicate_arr,
    int8_t* flag_arr, int32_t* progress_arr, int32_t* table_ctr,
    int32_t* idx_arr, int32_t* last_table, int* num_result_tuples) {
  auto table_predicate_to_flag_idx = ReconstructTablePredicateToFlagIdx(
      num_tables, table_predicate_to_flag_idx_len,
      table_predicate_to_flag_idx_arr);
  auto table_functions =
      ReconstructTableFunctions(num_tables, join_handler_fn_arr);
  auto tables_per_predicate =
      ReconstructTablesPerPredicate(num_predicates, tables_per_predicate_arr);
  auto cardinalities = ReconstructCardinalities(num_tables, idx_arr);
  ExecutionEngineFlags execution_engine{
      .join_handler_fn_arr = join_handler_fn_arr,
      .flag_arr = flag_arr,
      .progress_arr = progress_arr,
      .table_ctr = table_ctr,
      .idx_arr = idx_arr,
      .last_table = last_table,
      .num_result_tuples = num_result_tuples};

  JoinEnvironment environment(
      num_predicates, table_predicate_to_flag_idx, tables_per_predicate,
      cardinalities, table_functions, valid_tuple_handler, execution_engine);

  UctJoinAgent agent(num_tables, environment);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

}  // namespace kush::runtime