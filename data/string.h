#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace kush::data {

struct String {
  const char* data;
  uint32_t length;
};

void Create(String* vec, const char* data, uint32_t length);
void DeepCopy(String* src, String* dest);
void Free(String* src);
int8_t Contains(String* s1, String* s2);
int8_t EndsWith(String* s1, String* s2);
int8_t StartsWith(String* s1, String* s2);
int8_t Equals(String* s1, String* s2);
int8_t NotEquals(String* s1, String* s2);
int64_t Hash(String* s1);

}  // namespace kush::data