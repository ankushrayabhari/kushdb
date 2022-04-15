#include "runtime/aggregate_hash_table.h"

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

void Init(AggregateHashTable* ht, int64_t tuple_size,
          int64_t tuple_hash_offset) {
  ht->tuple_size = tuple_size;
  ht->tuple_hash_offset = tuple_hash_offset;

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

int32_t GetBlockIdx(uint64_t entry) { return entry & 0xFFFFFFFF; }

uint64_t ConstructEntry(uint16_t salt, uint16_t block_offset,
                        int32_t block_idx) {
  uint64_t salt_64 = salt;
  salt_64 <<= 48;
  uint64_t block_offset_64 = salt;
  block_offset_64 <<= 32;
  uint64_t block_idx_64 = block_idx;
  return salt_64 | block_offset_64 | block_idx_64;
}

void Resize(AggregateHashTable* ht) {
  auto capacity = ht->capacity * 2;
  auto mask = capacity - 1;

  auto entries = new uint64_t[capacity];
  memset(entries, 0, sizeof(uint64_t) * capacity);

  for (int32_t block_idx = 1; block_idx < ht->payload_block_size - 1;
       block_idx++) {
    for (uint16_t block_offset = 0; block_offset < BLOCK_SIZE;
         block_offset += ht->tuple_size) {
      auto ptr = ht->payload_block[block_idx] + block_offset;
      auto hash = *((uint64_t*)ptr + ht->tuple_hash_offset);
      uint16_t salt = hash >> 48;

      auto entry_idx = (int32_t)hash & mask;
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

}  // namespace kush::runtime::AggregateHashTable