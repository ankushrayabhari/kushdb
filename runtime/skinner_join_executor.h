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
    int32_t num_tables, int32_t num_general_predicates, int32_t num_eq_preds,
    const absl::flat_hash_map<std::pair<int, int>, int>* eq_pred_table_to_flag,
    const absl::flat_hash_map<std::pair<int, int>, int>*
        general_pred_table_to_flag,
    const absl::flat_hash_set<std::pair<int, int>>* table_connections,
    std::add_pointer<int32_t(int32_t, int8_t)>::type* join_handler_fn_arr,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler,
    int32_t num_flags, int8_t* flag_arr, int32_t* progress_arr,
    int32_t* table_ctr, int32_t* idx_arr, int32_t* last_table,
    int32_t* num_result_tuples, int32_t* offset_arr);

void ExecuteRecompilingSkinnerJoin(
    int32_t num_tables, int32_t* cardinality_arr,
    const absl::flat_hash_set<std::pair<int, int>>* table_connections,
    compile::RecompilingJoinTranslator* codegen, void** materialized_buffers,
    void** materialized_indexes, void* tuple_idx_table);

}  // namespace kush::runtime