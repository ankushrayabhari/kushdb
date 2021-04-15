#include "data/tuple_idx_table.h"

namespace kush::data {

std::unordered_set<std::vector<uint32_t>>* CreateTupleIdxTable() {
  return new std::unordered_set<std::vector<uint32_t>>;
}

void Insert(std::unordered_set<std::vector<uint32_t>>* ht, uint32_t* arr,
            uint32_t n) {
  ht->emplace(arr, arr + n);
}

void Free(std::unordered_set<std::vector<uint32_t>>* ht) { delete ht; }

void ForEach(std::unordered_set<std::vector<uint32_t>>* ht,
             ::std::add_pointer<void(uint32_t*)>::type cb) {
  for (auto& v : *ht) {
    cb(const_cast<uint32_t*>(v.data()));
  }
}

}  // namespace kush::data
