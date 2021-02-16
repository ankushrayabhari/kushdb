#include "data/vector.h"

#include <cstdint>
#include <cstdlib>

namespace kush::data {

void Create(Vector* vec, uint32_t element_size, uint32_t initial_capacity) {
  vec->element_size = element_size;
  vec->size = 0;
  vec->capacity = initial_capacity;
  vec->data = static_cast<char*>(malloc(initial_capacity * element_size));
}

void Grow(Vector* vec, uint32_t new_capacity) {
  vec->data =
      static_cast<char*>(realloc(vec->data, new_capacity * vec->element_size));
  vec->capacity = new_capacity;
}

void* PushBack(Vector* vec) {
  if (vec->size == vec->capacity) {
    Grow(vec, 2 * vec->capacity);
  }

  void* element = Get(vec, vec->size);
  vec->size += 1;
  return element;
}

void* Get(Vector* vec, uint32_t idx) {
  return vec->data + (idx * vec->element_size);
}

uint32_t Size(Vector* vec) { return vec->size; }

void Free(Vector* vec) { free(vec->data); }

void* Begin(Vector* vec) { return vec->data; }

void* End(Vector* vec) { return vec->data + (vec->size * vec->element_size); }

}  // namespace kush::data