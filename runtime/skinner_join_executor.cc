#include "runtime/skinner_join_executor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
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
        if (prev == &root_) {
          // first time executing this order at all
          return std::nullopt;
        }

        // all remaining tables have value cardinality - 1
        for (int j = i; j < order.size(); j++) {
          int table = order[j];
          last_completed_tuple_idx[table] = cardinalities_[table] - 1;
        }
        return last_completed_tuple_idx;
      } else {
        last_completed_tuple_idx[table] = curr->last_completed_tuple;
      }
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

    if (initial_last_completed_tuple.has_value()) {
      SetResumeProgress(order, initial_last_completed_tuple.value());
    }

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
    // TODO:
    return 0;
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
    execution_engine_.join_handler_fn_arr[order.size() - 1] =
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
    // Toggle predicate flags
    std::unordered_set<int> available_tables;
    std::unordered_set<int> executed_predicates;
    for (int table_idx : order) {
      available_tables.insert(table_idx);

      // all tables in available_tables have been joined
      // execute any non-executed predicate
      for (int predicate_idx = 0; predicate_idx < num_predicates_;
           predicate_idx++) {
        if (executed_predicates.find(predicate_idx) !=
            executed_predicates.end())
          continue;

        bool all_tables_available = true;
        for (int table : tables_per_predicate_[predicate_idx]) {
          all_tables_available =
              all_tables_available &&
              available_tables.find(table) != available_tables.end();
        }
        if (!all_tables_available) continue;

        executed_predicates.insert(predicate_idx);

        execution_engine_.flag_arr[table_predicate_to_flag_idx_[table_idx].at(
            predicate_idx)] = 1;
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

class StaticJoinAgent {
 public:
  StaticJoinAgent(int num_tables, JoinEnvironment& environment)
      : num_tables_(num_tables), environment_(environment) {}

  void Act() {
    // For now set order to just be a static one
    std::vector<int> order;
    for (int i = 0; i < num_tables_; i++) {
      order.push_back(i);
    }

    environment_.Execute(order);
  }

 private:
  int num_tables_;
  JoinEnvironment& environment_;
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

  StaticJoinAgent agent(num_tables, environment);

  while (!environment.IsComplete()) {
    agent.Act();
  }
}

}  // namespace kush::runtime