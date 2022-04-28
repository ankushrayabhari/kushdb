#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "runtime/allocator.h"
#include "runtime/column_index_bucket.h"
#include "runtime/string.h"

namespace kush::runtime::MemoryColumnIndex {

constexpr static int BLOCK_SIZE = 1 << 12;
constexpr static double LOAD_FACTOR = 1.5;

struct MemoryColumnIndex {
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

  Allocator* allocator;
};

void Init(MemoryColumnIndex* ht, uint16_t payload_size,
          uint64_t payload_hash_offset);

void AllocateNewPage(MemoryColumnIndex* ht);

void Resize(MemoryColumnIndex* ht);

void Free(MemoryColumnIndex* ht);

uint64_t ConstructEntry(uint16_t salt, uint16_t block_offset,
                        uint32_t block_idx);

void* GetPayload(MemoryColumnIndex* ht, uint32_t block_idx,
                 uint64_t block_offset);

void* GetEntry(MemoryColumnIndex* ht, uint32_t idx);

}  // namespace kush::runtime::MemoryColumnIndex