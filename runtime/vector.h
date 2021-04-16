#pragma once

#include <cstdint>
#include <type_traits>

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

void Sort(Vector* vec,
          ::std::add_pointer<bool(int8_t*, int8_t*)>::type comp_fn);

}  // namespace kush::data