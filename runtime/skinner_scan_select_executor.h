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
    int num_predicates, const std::vector<int32_t>* indexed_predicates,
    std::add_pointer<int32_t(int32_t, int8_t)>::type index_scan_fn,
    std::add_pointer<int32_t(int32_t, int8_t)>::type scan_fn,
    int32_t* num_handlers, std::add_pointer<int8_t()>::type* handlers,
    int32_t* idx);

}  // namespace kush::runtime