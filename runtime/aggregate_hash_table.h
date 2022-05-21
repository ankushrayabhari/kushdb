#pragma once

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

constexpr static int BLOCK_SIZE = 1 << 12;
constexpr static double LOAD_FACTOR = 1.5;

struct AggregateHashTable {
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
  uint16_t payload_size;
};

void Init(AggregateHashTable* ht, uint16_t payload_size,
          uint64_t payload_hash_offset);

void AllocateNewPage(AggregateHashTable* ht);

void Resize(AggregateHashTable* ht);

void Free(AggregateHashTable* ht);

uint64_t ConstructEntry(uint16_t salt, uint16_t block_offset,
                        uint32_t block_idx);

void* GetPayload(AggregateHashTable* ht, uint32_t block_idx,
                 uint64_t block_offset);

void* GetEntry(AggregateHashTable* ht, uint32_t idx);

int32_t ComputeBlockIdx(AggregateHashTable* ht, uint32_t idx);

int32_t ComputeBlockOffset(AggregateHashTable* ht, uint32_t idx);

int32_t Size(AggregateHashTable* ht);

}  // namespace kush::runtime::AggregateHashTable