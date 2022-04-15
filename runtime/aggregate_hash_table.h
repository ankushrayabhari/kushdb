#pragma once

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

constexpr static int BLOCK_SIZE = 1 << 12;
constexpr static double LOAD_FACTOR = 1.5;

struct AggregateHashTable {
  uint64_t tuple_size;
  uint64_t tuple_hash_offset;

  // Entry storage
  int32_t size;
  int32_t capacity;
  uint64_t mask;
  uint64_t* entries;

  // Tuple Storage
  uint8_t** payload_block;
  int32_t payload_block_capacity;
  int32_t payload_block_size;
  uint16_t last_payload_offset;
};

void Init(AggregateHashTable* ht, int64_t tuple_size,
          int64_t tuple_hash_offset);

void AllocateNewPage(AggregateHashTable* ht);

void Resize(AggregateHashTable* ht);

void Free(AggregateHashTable* ht);

uint16_t ComputeSalt(uint64_t hash);

int32_t ComputeMod(uint64_t hash, uint64_t mask);

}  // namespace kush::runtime::AggregateHashTable