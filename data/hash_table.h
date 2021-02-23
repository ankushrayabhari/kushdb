#pragma once

#include <cstdint>
#include <type_traits>

namespace kush::data {

// Represents a hash table of tuples, i.e. map<key, vector<tuple>>
struct HashTable {
  uint64_t element_size_;
  void* data_;
};

void Create(HashTable* ht, uint64_t element_size);

int8_t* Insert(HashTable* ht, int64_t hash);

void Get(HashTable* ht, int64_t hash,
         ::std::add_pointer<void(int8_t*)>::type callback);

void Free(HashTable* ht);

void HashCombine(int64_t* hash, int64_t v);

}  // namespace kush::data