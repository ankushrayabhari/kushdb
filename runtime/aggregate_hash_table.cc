#include "runtime/aggregate_hash_table.h"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace kush::runtime::AggregateHashTable {

void Init(AggregateHashTable* ht, uint64_t payload_size,
          uint64_t payload_hash_offset) {
  ht->payload_size = payload_size;
  ht->payload_hash_offset = payload_hash_offset;

  ht->capacity = 1024;
  ht->size = 0;
  auto entries = new uint64_t[ht->capacity];
  memset(entries, 0, sizeof(uint64_t) * ht->capacity);

  ht->payload_block_size = 1;
  ht->payload_block_capacity = 4;
  ht->payload_block = new uint8_t*[ht->payload_block_capacity];
  ht->payload_block[0] = nullptr;
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
}

uint32_t GetBlockIdx(uint64_t entry) { return entry & 0xFFFFFFFF; }

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
    for (uint16_t block_offset = 0; block_offset < end;
         block_offset += ht->payload_size) {
      auto ptr = ht->payload_block[block_idx] + block_offset;
      auto hash = *((uint64_t*)ptr + ht->payload_hash_offset);
      uint16_t salt = hash >> 48;

      auto entry_idx = (uint32_t)hash & mask;
      auto entry = entries[entry_idx];
      while (GetBlockIdx(entry) > 0) {
        entry_idx++;
        if (entry_idx >= capacity) {
          entry_idx = 0;
        }
      }

      entries[entry_idx] = ConstructEntry(salt, block_idx, block_offset);
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
                 uint16_t block_offset) {
  return ht->payload_block[block_idx] + block_offset;
}

void* GetEntry(AggregateHashTable* ht, uint32_t idx) {
  return &ht->entries[idx];
}

}  // namespace kush::runtime::AggregateHashTable