#pragma once

#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kush::runtime {

void ExecuteSkinnerJoin(int num_tables, int num_predicates,
                        std::add_pointer<int(int)>::type* join_handler_fn_arr,
                        std::add_pointer<int(int)>::type valid_tuple_handler,
                        int table_predicate_to_flag_idx_len,
                        int* table_predicate_to_flag_idx_arr,
                        int* tables_per_predicate_arr, int8_t* flag_arr);

}  // namespace kush::runtime