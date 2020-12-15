#pragma once

#include <typeindex>

namespace kush::util {

template <typename T, typename... Rest>
void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (HashCombine(seed, rest), ...);
}

}  // namespace kush::util

#define GENERATE_HASH_SPECIALIZATION(T)                           \
  template <>                                                     \
  struct std::hash<T> {                                           \
    std::size_t operator()(const T& t) const { return t.Hash(); } \
  };