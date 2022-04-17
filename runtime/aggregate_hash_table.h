#pragma once

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

constexpr static int BLOCK_SIZE = 1 << 12;
constexpr static double LOAD_FACTOR = 1.5;

struct AggregateHashTable {
  uint64_t payload_size;
  uint64_t payload_hash_offset;

  // Entry storage
  uint32_t size;
  uint32_t capacity;
  uint64_t mask;
  uint64_t* entries;

  // Tuple Storage
  uint8_t** payload_block;
  uint32_t payload_block_capacity;
  uint32_t payload_block_size;
  uint16_t last_payload_offset;
};

void Init(AggregateHashTable* ht, uint64_t payload_size,
          uint64_t payload_hash_offset);

void AllocateNewPage(AggregateHashTable* ht);

void Resize(AggregateHashTable* ht);

void Free(AggregateHashTable* ht);

uint64_t ConstructEntry(uint16_t salt, uint16_t block_offset,
                        uint32_t block_idx);

}  // namespace kush::runtime::AggregateHashTable