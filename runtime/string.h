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

void Copy(String* dest, String* src);
void Free(String* src);
bool Contains(String* s1, String* s2);
bool EndsWith(String* s1, String* s2);
bool StartsWith(String* s1, String* s2);
bool Equals(String* s1, String* s2);
bool NotEquals(String* s1, String* s2);
bool LessThan(String* s1, String* s2);
int64_t Hash(String* s1);

}  // namespace kush::data