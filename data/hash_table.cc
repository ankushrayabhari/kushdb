#include "data/hash_table.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "data/vector.h"

namespace kush::data {

using map_type = std::unordered_map<uint32_t, Vector>;

void Create(HashTable* ht, uint64_t element_size) {
  ht->element_size_ = element_size;
  ht->data_ = new map_type;
}

int8_t* Insert(HashTable* ht, uint32_t hash) {
  if (ht->data_->find(hash) == ht->data_->end()) {
    auto& new_vec = (*ht->data_)[hash];
    Create(&new_vec, ht->element_size_, 2);
  }

  auto& bucket = (*ht->data_)[hash];
  auto* data = PushBack(&bucket);
  return data;
}

Vector* GetBucket(HashTable* ht, uint32_t hash) {
  if (ht->data_->find(hash) == ht->data_->end()) {
    return nullptr;
  }

  auto& bucket = (*ht->data_)[hash];
  return &bucket;
}

void GetAllBuckets(HashTable* ht, BucketList* list) {
  map_type& bucket_map = *ht->data_;
  list->num_buckets = bucket_map.size();
  list->buckets = new Vector*[list->num_buckets];

  int i = 0;
  for (auto& [k, v] : bucket_map) {
    list->buckets[i++] = &v;
  }
}

Vector* GetBucketIdx(BucketList* l, uint32_t i) { return l->buckets[i]; }

uint32_t Size(BucketList* l) { return l->num_buckets; }

void Free(HashTable* ht) {
  for (auto& [k, v_list] : (*ht->data_)) {
    Free(&v_list);
  }

  delete ht->data_;
}

void Free(BucketList* ht) { delete[] ht->buckets; }

void HashCombine(uint32_t* hash, int64_t v) {
  *hash ^= v + 0x9e3779b9 + (*hash << 6) + (*hash >> 2);
}

}  // namespace kush::data