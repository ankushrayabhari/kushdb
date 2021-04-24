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

class Environment {
 public:
  virtual bool IsComplete() = 0;
};

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

  void Update(const std::vector<int32_t>& order,
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

bool IsJoinCompleted(const std::vector<int32_t>& last_completed_tuple,
                     const std::vector<int32_t>& cardinalities) {
  for (int i = 0; i < last_completed_tuple.size(); i++) {
    if (last_completed_tuple[i] != cardinalities[i] - 1) {
      return false;
    }
  }
  return true;
}

void SetJoinOrder(
    const std::vector<int>& order,
    const std::vector<std::add_pointer<int(int, int8_t)>::type>&
        table_functions,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    int32_t* last_table) {
  *last_table = order.back();

  for (int i = 0; i < order.size() - 1; i++) {
    // set the ith handler to i+1 function
    int current_table = order[i];
    int next_table = order[i + 1];

    join_handler_fn_arr[current_table] = table_functions[next_table];
  }

  // set last handler to be the valid tuple handler
  join_handler_fn_arr[order.size() - 1] = valid_tuple_handler;
}

std::vector<int32_t> ComputeLastCompletedTuple(
    const std::vector<int32_t>& order,
    const std::vector<int32_t>& cardinalities, bool should_decrement,
    int32_t table_ctr, int* idx_arr) {
  std::vector<int32_t> last_completed_tuple(order.size(), 0);

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

void SetProgress(const std::vector<int32_t>& order,
                 const std::vector<int32_t>& last_completed_tuple,
                 int32_t* progress_arr, int32_t* table_ctr) {
  for (int table = 0; table < last_completed_tuple.size(); table++) {
    progress_arr[table] = last_completed_tuple[table];
  }
  *table_ctr = order[0];
}

void TogglePredicateFlags(
    const std::vector<int>& order, int num_predicates,
    const std::vector<std::unordered_set<int>>& tables_per_predicate,
    const std::vector<std::unordered_map<int, int>>&
        table_predicate_to_flag_idx,
    int8_t* flag_arr) {
  // Toggle predicate flags
  std::unordered_set<int> available_tables;
  std::unordered_set<int> executed_predicates;
  for (int table_idx : order) {
    available_tables.insert(table_idx);

    // all tables in available_tables have been joined
    // execute any non-executed predicate
    for (int predicate_idx = 0; predicate_idx < num_predicates;
         predicate_idx++) {
      if (executed_predicates.find(predicate_idx) != executed_predicates.end())
        continue;

      bool all_tables_available = true;
      for (int table : tables_per_predicate[predicate_idx]) {
        all_tables_available =
            all_tables_available &&
            available_tables.find(table) != available_tables.end();
      }
      if (!all_tables_available) continue;

      executed_predicates.insert(predicate_idx);

      flag_arr[table_predicate_to_flag_idx[table_idx].at(predicate_idx)] = 1;
    }
  }
}

void ExecuteSkinnerJoin(
    int32_t num_tables, int32_t num_predicates,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    int32_t table_predicate_to_flag_idx_len,
    int32_t* table_predicate_to_flag_idx_arr, int32_t* tables_per_predicate_arr,
    int8_t* flag_arr, int32_t* progress_arr, int32_t* table_ctr,
    int32_t* idx_arr, int32_t* last_table) {
  // Reconstruct join/predicate graph
  std::vector<std::unordered_map<int, int>> table_predicate_to_flag_idx(
      num_tables);
  for (int i = 0; i < table_predicate_to_flag_idx_len; i += 3) {
    int table = table_predicate_to_flag_idx_arr[i];
    int predicate = table_predicate_to_flag_idx_arr[i + 1];
    int flag_idx = table_predicate_to_flag_idx_arr[i + 2];
    table_predicate_to_flag_idx[table][predicate] = flag_idx;
  }

  std::vector<std::add_pointer<int(int, int8_t)>::type> table_functions;
  for (int i = 0; i < num_tables; i++) {
    table_functions.push_back(join_handler_fn_arr[i]);
  }

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

  std::vector<int32_t> cardinalities;
  for (int i = 0; i < num_tables; i++) {
    cardinalities.push_back(idx_arr[i]);
  }

  bool initial_execution = true;
  int32_t budget = 10000;

  std::vector<int32_t> last_completed_tuple(num_tables, -1);

  while (true) {
    // For now set order to just be a static one
    std::vector<int> order;
    for (int i = 0; i < num_tables; i++) {
      order.push_back(i);
    }

    SetJoinOrder(order, table_functions, valid_tuple_handler,
                 join_handler_fn_arr, last_table);
    TogglePredicateFlags(order, num_predicates, tables_per_predicate,
                         table_predicate_to_flag_idx, flag_arr);
    SetProgress(order, last_completed_tuple, progress_arr, table_ctr);

    bool should_decrement =
        table_functions[order[0]](budget, initial_execution ? 0 : 1) == -2;

    last_completed_tuple = ComputeLastCompletedTuple(
        order, cardinalities, should_decrement, *table_ctr, idx_arr);

    initial_execution = false;

    if (IsJoinCompleted(last_completed_tuple, cardinalities)) {
      break;
    }
  }
}

}  // namespace kush::runtime