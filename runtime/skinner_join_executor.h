#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/translators/recompiling_join_translator.h"

namespace kush::runtime {

void ExecutePermutableSkinnerJoin(
    int32_t num_tables, int32_t num_predicates,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    int32_t table_predicate_to_flag_idx_len,
    int32_t* table_predicate_to_flag_idx_arr, int32_t* tables_per_predicate_arr,
    int8_t* flag_arr, int32_t* progress_arr, int32_t* table_ctr,
    int32_t* idx_arr, int32_t* last_table, int32_t* num_result_tuples,
    int32_t* offset_arr);

void ExecuteRecompilingSkinnerJoin(
    int32_t num_tables, int32_t* cardinality_arr,
    absl::flat_hash_set<std::pair<int, int>>* table_connections,
    compile::RecompilingJoinTranslator* codegen, void** materialized_buffers,
    void** materialized_indexes, void* tuple_idx_table);

}  // namespace kush::runtime