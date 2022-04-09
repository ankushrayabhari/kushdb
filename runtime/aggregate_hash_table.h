#pragma once

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

struct HashTableEntry {
  uint16_t salt;
  uint16_t block_offset;
  int32_t block_idx;
};

struct AggregateHashTable {
  uint64_t tuple_size;
  uint64_t tuple_hash_offset;

  // Entry storage
  int32_t size;
  int32_t capacity;
  uint64_t mask;
  HashTableEntry* entries;

  // Tuple Storage
  uint8_t** payload_block;
  int32_t payload_block_capacity;
  int32_t payload_block_size;
  uint16_t last_payload_offset;
};

void Init(AggregateHashTable* ht, int64_t tuple_size,
          int64_t tuple_hash_offset);

uint8_t* AllocateNewPage(AggregateHashTable* ht);

void Resize(AggregateHashTable* ht);

void Free(AggregateHashTable* ht);

}  // namespace kush::runtime::AggregateHashTable