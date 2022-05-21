#include "runtime/aggregate_hash_table.h"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace kush::runtime::AggregateHashTable {

void Init(AggregateHashTable* ht, uint16_t payload_size,
          uint64_t payload_hash_offset) {
  ht->payload_size = payload_size;
  ht->payload_hash_offset = payload_hash_offset;

  ht->capacity = 1024;
  ht->mask = ht->capacity - 1;
  ht->size = 0;
  ht->entries = new uint64_t[ht->capacity];
  memset(ht->entries, 0, sizeof(uint64_t) * ht->capacity);

  ht->payload_block_size = 2;
  ht->payload_block_capacity = 4;
  ht->payload_block = new uint8_t*[ht->payload_block_capacity];
  // skip block 0 bc use it as empty slot marker
  ht->payload_block[0] = nullptr;
  // block 1
  ht->payload_block[1] = new uint8_t[BLOCK_SIZE];
  ht->last_payload_offset = 0;
}

void AllocateNewPage(AggregateHashTable* ht) {
  if (ht->payload_block_size == ht->payload_block_capacity) {
    auto new_capacity = ht->payload_block_capacity * 2;
    auto new_payload_block = new uint8_t*[new_capacity];
    memcpy(new_payload_block, ht->payload_block,
           sizeof(uint8_t*) * ht->payload_block_capacity);
    delete[] ht->payload_block;
    ht->payload_block = new_payload_block;
    ht->payload_block_capacity = new_capacity;
  }

  ht->payload_block[ht->payload_block_size] = new uint8_t[BLOCK_SIZE];
  ht->payload_block_size++;
  ht->last_payload_offset = 0;
}

uint64_t ConstructEntry(uint16_t salt, uint16_t block_offset,
                        uint32_t block_idx) {
  uint64_t salt_64 = salt;
  uint64_t block_offset_64 = block_offset;
  uint64_t block_idx_64 = block_idx;

  uint64_t output = 0;
  output |= salt_64 << 48;
  output |= block_offset_64 << 32;
  output |= block_idx_64;
  return output;
}

void Resize(AggregateHashTable* ht) {
  auto capacity = ht->capacity * 2;
  auto mask = capacity - 1;

  auto entries = new uint64_t[capacity];
  memset(entries, 0, sizeof(uint64_t) * capacity);

  for (uint32_t block_idx = 1; block_idx < ht->payload_block_size;
       block_idx++) {
    const auto end = block_idx == ht->payload_block_size - 1
                         ? ht->last_payload_offset
                         : BLOCK_SIZE;
    for (uint16_t block_offset = 0; block_offset + ht->payload_size <= end;
         block_offset += ht->payload_size) {
      auto hash_ptr =
          ht->payload_block[block_idx] + block_offset + ht->payload_hash_offset;
      auto hash = *(uint64_t*)(hash_ptr);
      uint16_t salt = hash >> 48;

      uint32_t entry_idx = hash & mask;
      while ((entries[entry_idx] & 0xFFFFFFFF) > 0) {
        entry_idx = (entry_idx + 1) & mask;
      }

      entries[entry_idx] = ConstructEntry(salt, block_offset, block_idx);
    }
  }

  delete[] ht->entries;
  ht->entries = entries;
  ht->capacity = capacity;
  ht->mask = mask;
}

void Free(AggregateHashTable* ht) {
  delete[] ht->entries;

  for (int i = 0; i < ht->payload_block_size; i++) {
    delete[] ht->payload_block[i];
  }
  delete[] ht->payload_block;
}

void* GetPayload(AggregateHashTable* ht, uint32_t block_idx,
                 uint64_t block_offset) {
  return ht->payload_block[block_idx] + block_offset;
}

void* GetEntry(AggregateHashTable* ht, uint32_t idx) {
  return &ht->entries[idx];
}

int32_t ComputeBlockIdx(AggregateHashTable* ht, uint32_t idx) {
  auto tuples_per_block = BLOCK_SIZE / ht->payload_size;
  return idx / tuples_per_block + 1;
}

int32_t ComputeBlockOffset(AggregateHashTable* ht, uint32_t idx) {
  auto tuples_per_block = BLOCK_SIZE / ht->payload_size;
  return ht->payload_size * (idx % tuples_per_block);
}

int32_t Size(AggregateHashTable* ht) {
  auto tuples_per_block = BLOCK_SIZE / ht->payload_size;
  int32_t output = 0;

  // skip last block and block 0
  int32_t full_blocks = ht->payload_block_size - 2;
  if (full_blocks > 0) {
    output += full_blocks * tuples_per_block;
  }

  // handle last block
  output += ht->last_payload_offset / ht->payload_size;

  return output;
}

}  // namespace kush::runtime::AggregateHashTable