#pragma once

#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "runtime/vector.h"

namespace kush::runtime::HashTable {

// Represents a hash table of tuples, i.e. map<key, vector<tuple>>
struct HashTable {
  int64_t element_size_;
  std::unordered_map<int32_t, runtime::Vector::Vector>* data_;
};

struct BucketList {
  int32_t num_buckets;
  runtime::Vector::Vector** buckets;
};

void Create(HashTable* ht, int64_t element_size);

int8_t* Insert(HashTable* ht, int32_t hash);

runtime::Vector::Vector* GetBucket(HashTable* ht, int32_t hash);

void GetAllBuckets(HashTable* ht, BucketList* l);

void Free(HashTable* ht);

runtime::Vector::Vector* GetBucketIdx(BucketList* l, int32_t i);

int32_t BucketListSize(BucketList* l);

void BucketListFree(BucketList* bl);

void HashCombine(int32_t* hash, int64_t v);

}  // namespace kush::runtime::HashTable