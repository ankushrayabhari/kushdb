#pragma once

#include <cstdint>

namespace kush::data {

struct Vector {
  uint32_t element_size;
  uint32_t size;
  uint32_t capacity;
  char* data;
};

void Create(Vector* vec, uint32_t element_size, uint32_t initial_capacity);

void* PushBack(Vector* vec);

void* Get(Vector* vec, uint32_t idx);

uint32_t Size(Vector* vec);

void* Begin(Vector* vec);

void Free(Vector* vec);

}  // namespace kush::data