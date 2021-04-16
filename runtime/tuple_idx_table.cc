#include "runtime/tuple_idx_table.h"

namespace kush::data {

std::unordered_set<std::vector<uint32_t>>* CreateTupleIdxTable() {
  return new std::unordered_set<std::vector<uint32_t>>;
}

void Insert(std::unordered_set<std::vector<uint32_t>>* ht, uint32_t* arr,
            uint32_t n) {
  ht->emplace(arr, arr + n);
}

void Free(std::unordered_set<std::vector<uint32_t>>* ht) { delete ht; }

uint32_t Size(std::unordered_set<std::vector<uint32_t>>* ht) {
  return ht->size();
}

void Begin(std::unordered_set<std::vector<uint32_t>>* ht,
           std::unordered_set<std::vector<uint32_t>>::const_iterator** it) {
  *it = new std::unordered_set<std::vector<uint32_t>>::const_iterator(
      ht->cbegin());
}

void Free(std::unordered_set<std::vector<uint32_t>>::const_iterator** it) {
  delete *it;
}

void Increment(std::unordered_set<std::vector<uint32_t>>::const_iterator** it) {
  (*it)->operator++();
}

uint32_t* Get(std::unordered_set<std::vector<uint32_t>>::const_iterator** it) {
  return const_cast<uint32_t*>((*it)->operator*().data());
}

}  // namespace kush::data
