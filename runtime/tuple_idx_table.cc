#include "runtime/tuple_idx_table.h"

#include <iostream>

namespace kush::runtime::TupleIdxTable {

std::unordered_set<std::vector<int32_t>>* Create() {
  return new std::unordered_set<std::vector<int32_t>>;
}

void Insert(std::unordered_set<std::vector<int32_t>>* ht, int32_t* arr,
            int32_t n) {
  ht->emplace(arr, arr + n);
}

void Free(std::unordered_set<std::vector<int32_t>>* ht) { delete ht; }

int32_t Size(std::unordered_set<std::vector<int32_t>>* ht) {
  return ht->size();
}

void Begin(std::unordered_set<std::vector<int32_t>>* ht,
           std::unordered_set<std::vector<int32_t>>::const_iterator** it) {
  *it = new std::unordered_set<std::vector<int32_t>>::const_iterator(
      ht->cbegin());
}

void FreeIt(std::unordered_set<std::vector<int32_t>>::const_iterator** it) {
  delete *it;
}

void Increment(std::unordered_set<std::vector<int32_t>>::const_iterator** it) {
  (*it)->operator++();
}

int32_t* Get(std::unordered_set<std::vector<int32_t>>::const_iterator** it) {
  return const_cast<int32_t*>((*it)->operator*().data());
}

}  // namespace kush::runtime::TupleIdxTable
