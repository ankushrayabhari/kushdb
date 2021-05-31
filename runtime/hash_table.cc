#include "runtime/hash_table.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "runtime/vector.h"

namespace kush::runtime::HashTable {

using map_type = std::unordered_map<int32_t, runtime::Vector::Vector>;

void Create(HashTable* ht, int64_t element_size) {
  ht->element_size_ = element_size;
  ht->data_ = new map_type;
}

int8_t* Insert(HashTable* ht, int32_t hash) {
  if (ht->data_->find(hash) == ht->data_->end()) {
    auto& new_vec = (*ht->data_)[hash];
    runtime::Vector::Create(&new_vec, ht->element_size_, 2);
  }

  auto& bucket = (*ht->data_)[hash];
  auto* data = runtime::Vector::PushBack(&bucket);
  return data;
}

runtime::Vector::Vector* GetBucket(HashTable* ht, int32_t hash) {
  if (ht->data_->find(hash) == ht->data_->end()) {
    return nullptr;
  }

  auto& bucket = (*ht->data_)[hash];
  return &bucket;
}

void GetAllBuckets(HashTable* ht, BucketList* list) {
  map_type& bucket_map = *ht->data_;
  list->num_buckets = bucket_map.size();
  list->buckets = new runtime::Vector::Vector*[list->num_buckets];

  int i = 0;
  for (auto& [k, v] : bucket_map) {
    list->buckets[i++] = &v;
  }
}

runtime::Vector::Vector* GetBucketIdx(BucketList* l, int32_t i) {
  return l->buckets[i];
}

int32_t BucketListSize(BucketList* l) { return l->num_buckets; }

void Free(HashTable* ht) {
  for (auto& [k, v_list] : (*ht->data_)) {
    Free(&v_list);
  }

  delete ht->data_;
}

void BucketListFree(BucketList* ht) { delete[] ht->buckets; }

void HashCombine(int32_t* hash, int64_t v) {
  *hash ^= v + 0x9e3779b9 + (*hash << 6) + (*hash >> 2);
}

}  // namespace kush::runtime::HashTable