#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace kush::runtime {

void ExecutePermutableSkinnerScanSelect(
    absl::flat_hash_set<int>* index_executable_predicates,
    std::add_pointer<int32_t(int32_t, int32_t)>::type main_fn,
    int32_t* index_array, int32_t* index_array_size, int32_t num_predicates,
    int32_t* progress_idx);

std::add_pointer<bool()>::type GetPredicateFn(
    std::add_pointer<bool()>::type** preds, int idx);

}  // namespace kush::runtime