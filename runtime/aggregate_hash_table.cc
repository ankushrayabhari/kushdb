#include "runtime/aggregate_hash_table.h"

#include <cstdint>
#include <cstring>

namespace kush::runtime::AggregateHashTable {

constexpr static int BLOCK_SIZE = 1 << 12;

void Init(AggregateHashTable* ht, int64_t tuple_size,
          int64_t tuple_hash_offset) {
  ht->tuple_size = tuple_size;
  ht->tuple_hash_offset = tuple_hash_offset;

  ht->capacity = 1024;
  ht->size = 0;
  auto entries = new HashTableEntry[ht->capacity];
  memset(entries, 0, sizeof(HashTableEntry) * ht->capacity);

  ht->payload_block_size = 1;
  ht->payload_block_capacity = 4;
  ht->payload_block = new uint8_t*[ht->payload_block_capacity];
  ht->payload_block[0] = nullptr;
  ht->last_payload_offset = 0;
}

uint8_t* AllocateNewPage(AggregateHashTable* ht) {
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
  return ht->payload_block[ht->payload_block_size];
}

void Resize(AggregateHashTable* ht) {
  auto capacity = ht->capacity * 2;
  auto mask = capacity - 1;

  auto entries = new HashTableEntry[capacity];
  memset(entries, 0, sizeof(HashTableEntry) * capacity);

  for (int32_t block_idx = 1; block_idx < ht->payload_block_size - 1;
       block_idx++) {
    for (uint16_t block_offset = 0; block_offset < BLOCK_SIZE;
         block_offset += ht->tuple_size) {
      auto ptr = ht->payload_block[block_idx] + block_offset;
      auto hash = *((uint64_t*)ptr + ht->tuple_hash_offset);

      auto entry_idx = (int32_t)hash & mask;
      while (entries[entry_idx].block_idx > 0) {
        entry_idx++;
        if (entry_idx >= capacity) {
          entry_idx = 0;
        }
      }

      entries[entry_idx].salt = hash >> 48;
      entries[entry_idx].block_idx = block_idx;
      entries[entry_idx].block_offset = block_offset;
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