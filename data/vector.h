#pragma once

#include <cstdint>

namespace kush::data {

struct Vector {
  uint64_t element_size;
  uint32_t size;
  uint32_t capacity;
  int8_t* data;
};

void Create(Vector* vec, uint64_t element_size, uint32_t initial_capacity);

int8_t* PushBack(Vector* vec);

int8_t* Get(Vector* vec, uint32_t idx);

uint32_t Size(Vector* vec);

void Free(Vector* vec);

}  // namespace kush::data