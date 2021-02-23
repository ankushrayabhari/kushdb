#include "data/vector.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

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

uint32_t Size(Vector* vec) {
  if (vec == nullptr) {
    return 0;
  }
  return vec->size;
}

void Free(Vector* vec) { free(vec->data); }

void Merge(Vector* vec,
           ::std::add_pointer<int8_t(int8_t*, int8_t*)>::type comp_fn, int l,
           int m, int r) {
  int n1 = m - l + 1;
  int n2 = r - m;

  Vector v1, v2;
  Create(&v1, vec->element_size, n1);
  Create(&v2, vec->element_size, n2);
  memcpy(v1.data, Get(vec, l), n1 * vec->element_size);
  memcpy(v2.data, Get(vec, m + 1), n2 * vec->element_size);

  int i = 0;
  int j = 0;
  int k = l;

  while (i < n1 && j < n2) {
    auto* left_i = Get(&v1, i);
    auto* right_j = Get(&v2, j);
    auto* dest = Get(vec, k);

    if (comp_fn(left_i, right_j) != 0) {
      memcpy(dest, left_i, vec->element_size);
      i++;
    } else {
      memcpy(dest, right_j, vec->element_size);
      j++;
    }

    k++;
  }

  while (i < n1) {
    auto* left_i = Get(&v1, i);
    auto* dest = Get(vec, k);
    memcpy(dest, left_i, vec->element_size);
    i++;
    k++;
  }

  while (j < n2) {
    auto* right_j = Get(&v2, j);
    auto* dest = Get(vec, k);
    memcpy(dest, right_j, vec->element_size);
    j++;
    k++;
  }

  Free(&v1);
  Free(&v2);
}

void MergeSort(Vector* vec,
               ::std::add_pointer<int8_t(int8_t*, int8_t*)>::type comp_fn,
               int l, int r) {
  if (l >= r) {
    return;  // returns recursively
  }
  int m = l + (r - l) / 2;
  MergeSort(vec, comp_fn, l, m);
  MergeSort(vec, comp_fn, m + 1, r);
  Merge(vec, comp_fn, l, m, r);
}

void Sort(Vector* vec,
          ::std::add_pointer<int8_t(int8_t*, int8_t*)>::type comp_fn) {
  return MergeSort(vec, comp_fn, 0, vec->size - 1);
}

}  // namespace kush::data