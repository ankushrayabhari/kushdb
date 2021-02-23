#include "data/string.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace kush::data {

void Create(String* vec, const char* data, uint32_t length) {
  vec->data = data;
  vec->length = length;
}

void DeepCopy(String* src, String* dest) {
  auto* cpy = (char*)malloc(sizeof(char) * src->length);
  memcpy(cpy, src->data, src->length);

  dest->data = cpy;
  dest->length = src->length;
}

void Free(String* src) { free((void*)src->data); }

int8_t Contains(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1.find(sv2) != -1;
}

int8_t EndsWith(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);

  if (sv1.length() >= sv2.length()) {
    return (0 == sv1.compare(sv1.length() - sv2.length(), sv2.length(), sv2));
  } else {
    return false;
  }
}

int8_t StartsWith(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1.rfind(sv2, 0) == 0;
}

int8_t Equals(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1 == sv2;
}

int8_t NotEquals(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1 != sv2;
}

int64_t Hash(String* s1) {
  auto sv1 = std::string_view(s1->data, s1->length);
  return std::hash<std::string_view>{}(sv1);
}

}  // namespace kush::data