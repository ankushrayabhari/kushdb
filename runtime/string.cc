#include "runtime/string.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace kush::runtime::String {

void Copy(String* dest, String* src) {
  auto* cpy = (char*)malloc(sizeof(char) * src->length);
  memcpy(cpy, src->data, src->length);

  dest->data = cpy;
  dest->length = src->length;
}

void Free(String* src) { free((void*)src->data); }

bool Contains(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1.find(sv2) != std::string_view::npos;
}

bool EndsWith(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);

  if (sv1.length() >= sv2.length()) {
    return (0 == sv1.compare(sv1.length() - sv2.length(), sv2.length(), sv2));
  } else {
    return false;
  }
}

bool StartsWith(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1.rfind(sv2, 0) == 0;
}

bool Equals(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1 == sv2;
}

bool NotEquals(String* s1, String* s2) {
  auto sv1 = std::string_view(s1->data, s1->length);
  auto sv2 = std::string_view(s2->data, s2->length);
  return sv1 != sv2;
}

bool LessThan(String* s1, String* s2) {
  return std::string_view(s1->data, s1->length) <
         std::string_view(s2->data, s2->length);
}

int64_t Hash(String* s1) {
  auto sv1 = std::string_view(s1->data, s1->length);
  return std::hash<std::string_view>{}(sv1);
}

}  // namespace kush::runtime::String