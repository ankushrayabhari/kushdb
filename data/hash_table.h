#pragma once

#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "data/vector.h"

namespace kush::data {

// Represents a hash table of tuples, i.e. map<key, vector<tuple>>
struct HashTable {
  uint64_t element_size_;
  std::unordered_map<uint32_t, Vector>* data_;
};

struct BucketList {
  uint32_t num_buckets;
  Vector** buckets;
};

void Create(HashTable* ht, uint64_t element_size);

int8_t* Insert(HashTable* ht, uint32_t hash);

Vector* GetBucket(HashTable* ht, uint32_t hash);

void GetAllBuckets(HashTable* ht, BucketList* l);

Vector* GetBucketIdx(BucketList* l, uint32_t i);

uint32_t Size(BucketList* l);

void Free(HashTable* ht);

void HashCombine(uint32_t* hash, int64_t v);

}  // namespace kush::data