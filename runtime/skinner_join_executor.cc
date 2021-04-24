#include "runtime/skinner_join_executor.h"

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kush::runtime {

bool IsJoinCompleted(const std::vector<int>& order,
                     const std::vector<int32_t>& cardinalities,
                     int32_t* idx_arr) {
  int left_most_table = order[0];
  if (idx_arr[left_most_table] == cardinalities[left_most_table]) {
    return true;
  }
  return false;
}

void SetJoinHandlerOrder(
    const std::vector<int>& order,
    const std::vector<std::add_pointer<int(int, int8_t)>::type>&
        table_functions,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr) {
  // Set an initial static join order to be the tables that appear
  for (int i = 0; i < order.size() - 1; i++) {
    // set the ith handler to i+1 function
    int current_table = order[i];
    int next_table = order[i + 1];

    join_handler_fn_arr[current_table] = table_functions[next_table];
  }

  // set last handler to be the valid tuple handler
  join_handler_fn_arr[order.size() - 1] = valid_tuple_handler;
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

  // For now set order to just be a static one
  std::vector<int> order;
  for (int i = 0; i < num_tables; i++) {
    order.push_back(i);
  }
  *last_table = order.back();

  SetJoinHandlerOrder(order, table_functions, valid_tuple_handler,
                      join_handler_fn_arr);
  TogglePredicateFlags(order, num_predicates, tables_per_predicate,
                       table_predicate_to_flag_idx, flag_arr);

  // Execute the join order by calling the first table's function
  bool should_decrement = table_functions[order[0]](INT32_MAX, 0) == -2;
}

}  // namespace kush::runtime