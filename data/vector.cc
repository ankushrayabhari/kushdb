#include "data/vector.h"

#include <cstdint>
#include <cstdlib>

namespace kush::data {

void Create(Vector* vec, uint64_t element_size, uint32_t initial_capacity) {
  vec->element_size = element_size;
  vec->size = 0;
  vec->capacity = initial_capacity;
  vec->data = static_cast<int8_t*>(malloc(initial_capacity * element_size));
}

void Grow(Vector* vec, uint32_t new_capacity) {
  vec->data = static_cast<int8_t*>(
      realloc(vec->data, new_capacity * vec->element_size));
  vec->capacity = new_capacity;
}

int8_t* PushBack(Vector* vec) {
  if (vec->size == vec->capacity) {
    Grow(vec, 2 * vec->capacity);
  }

  auto* element = Get(vec, vec->size);
  vec->size += 1;
  return element;
}

int8_t* Get(Vector* vec, uint32_t idx) {
  return vec->data + (idx * vec->element_size);
}

uint32_t Size(Vector* vec) { return vec->size; }

void Free(Vector* vec) { free(vec->data); }

}  // namespace kush::data