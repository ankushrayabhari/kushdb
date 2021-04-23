#include "runtime/skinner_join_executor.h"

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kush::runtime {

void ExecuteSkinnerJoin(
    int num_tables, int num_predicates,
    std::add_pointer<int(int, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int(int, int8_t)>::type valid_tuple_handler,
    int table_predicate_to_flag_idx_len, int* table_predicate_to_flag_idx_arr,
    int* tables_per_predicate_arr, int8_t* flag_arr, int* progress_arr) {
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

  // Set an initial static join order to be the tables that appear
  for (int i = 0; i < num_tables - 1; i++) {
    // set the ith handler to i+1 function
    join_handler_fn_arr[i] = table_functions[i + 1];
  }
  // set last handler to be the valid tuple handler
  join_handler_fn_arr[num_tables - 1] = valid_tuple_handler;

  // Toggle predicate flags
  std::unordered_set<int> available_tables;
  std::unordered_set<int> executed_predicates;
  for (int table_idx = 0; table_idx < num_tables; table_idx++) {
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

      flag_arr[table_predicate_to_flag_idx[table_idx][predicate_idx]] = 1;
    }
  }

  // Initial budget = 10000

  // Execute the join order by calling the first function
  table_functions[0](INT32_MAX, 0);
}

}  // namespace kush::runtime