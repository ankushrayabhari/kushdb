#include "data/hash_table.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "absl/container/flat_hash_map.h"

namespace kush::data {

using map_type = std::unordered_map<int64_t, std::vector<int8_t*>>;

void Create(HashTable* ht, uint64_t element_size) {
  ht->element_size_ = element_size;
  ht->data_ = malloc(sizeof(map_type));
}

int8_t* Insert(HashTable* ht, int64_t hash) {
  auto& x = *static_cast<map_type*>(ht->data_);
  int8_t* data = (int8_t*)malloc(ht->element_size_);
  x[hash].push_back(data);
  return data;
}

void Get(HashTable* ht, int64_t hash,
         ::std::add_pointer<void(int8_t*)>::type callback) {
  auto& x = *static_cast<map_type*>(ht->data_);

  for (auto v : x[hash]) {
    callback(v);
  }
}

void Free(HashTable* ht) {
  auto* x = static_cast<map_type*>(ht->data_);

  for (auto& [k, v_list] : *x) {
    for (auto* v : v_list) {
      free(v);
    }
  }
  delete x;
}

void HashCombine(int64_t* hash, int64_t v) {
  *hash ^= v + 0x9e3779b9 + (*hash << 6) + (*hash >> 2);
}

}  // namespace kush::data