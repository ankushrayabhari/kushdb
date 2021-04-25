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

namespace std {
template <>
struct hash<std::vector<int>> {
  size_t operator()(const std::vector<int>& x) const {
    std::size_t seed(0);
    for (const int v : x) {
      seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}  // namespace std

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
      : finished_(false), cardinalities_(cardinalities) {}

  bool IsComplete() { return finished_; }

  std::optional<std::vector<int32_t>> GetLastCompletedTupleIdx(
      const std::vector<int>& order) {
    auto it = order_to_last_completed_tuple.find(order);
    if (it == order_to_last_completed_tuple.end()) {
      return std::nullopt;
    }

    return it->second;
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

    order_to_last_completed_tuple[order] = last_completed_tuple_idx;
  }

 private:
  bool finished_;
  std::vector<int32_t> cardinalities_;
  std::unordered_map<std::vector<int>, std::vector<int>>
      order_to_last_completed_tuple;
};

struct ExecutionEngineFlags {
  std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr;
  int8_t* flag_arr;
  int32_t* progress_arr;
  int32_t* table_ctr;
  int32_t* idx_arr;
  int32_t* last_table;
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
  double Reward(const std::vector<int32_t> initial_last_completed_tuple,
                const std::vector<int32_t> final_last_completed_tuple) {
    // TODO: fix this
    return 0.5;
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

        std::cout << table << ' ' << predicate_idx << std::endl;
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
        // rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
        rng_(0) {
    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
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
        // rng_(std::chrono::system_clock::now().time_since_epoch().count()) {
        rng_(0) {
    for (int i = 0; i < num_actions_; i++) {
      priority_actions_.push_back(i);
    }

    joined_tables_.insert(joined_table);

    auto it = std::find(unjoined_tables_.begin(), unjoined_tables_.end(),
                        joined_table);
    unjoined_tables_.erase(it);

    int action = 0;
    for (int table : unjoined_tables_) {
      table_per_action_[action++] = table;
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

    std::shuffle(unjoined_tables_.begin(), unjoined_tables_.end(), rng_);

    auto it = unjoined_tables_.begin();
    for (int level = tree_level_ + 1; level < num_tables_; ++level) {
      int table = *it++;

      if (table == last_table) {
        table = *it++;
      }

      order[level] = table;
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
  std::vector<int> unjoined_tables_;
  std::vector<int> table_per_action_;
  std::default_random_engine rng_;

  static constexpr double EXPLORATION_WEIGHT_ = 1E-5;
};

class SwitchingRandomStaticJoinAgent {
 public:
  SwitchingRandomStaticJoinAgent(int num_tables, JoinEnvironment& environment)
      : round_ctr_(0), environment_(environment) {
    for (int i = 0; i < num_tables; i++) {
      order1_.push_back(i);
      order2_.push_back(i);
    }

    std::default_random_engine rng(0);
    std::shuffle(order1_.begin(), order1_.end(), rng);
    std::shuffle(order2_.begin(), order2_.end(), rng);
  }

  void Act() {
    if (round_ctr_ % 2 == 0) {
      environment_.Execute(order1_);
    } else {
      environment_.Execute(order2_);
    }
    round_ctr_++;
  }

 private:
  int round_ctr_;
  JoinEnvironment& environment_;
  std::vector<int> order1_, order2_;
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
    int32_t* idx_arr, int32_t* last_table) {
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
      .last_table = last_table};

  JoinEnvironment environment(
      num_predicates, table_predicate_to_flag_idx, tables_per_predicate,
      cardinalities, table_functions, valid_tuple_handler, execution_engine);

  SwitchingRandomStaticJoinAgent agent(num_tables, environment);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

}  // namespace kush::runtime