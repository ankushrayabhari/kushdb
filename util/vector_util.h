#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace kush::util {

// Utility function to create a vector of unique_ptr
// https://stackoverflow.com/a/29975225

// get the first type in a pack, if it exists:
template <class... Ts>
struct first {};
template <class T, class... Ts>
struct first<T, Ts...> {
  using type = T;
};
template <class... Ts>
using first_t = typename first<Ts...>::type;

// build the return type:
template <class T0, class... Ts>
using vector_T =
    typename std::conditional<std::is_same<T0, void>::value,
                              typename std::decay<first_t<Ts...>>::type,
                              T0>::type;
template <class T0, class... Ts>
using vector_t = std::vector<vector_T<T0, Ts...>>;

// make a vector, non-empty arg case:
template <class T0 = void, class... Ts, class R = vector_t<T0, Ts...>>
R MakeVector(Ts&&... ts) {
  R retval;
  retval.reserve(sizeof...(Ts));  // we know how many elements
  // array unpacking trick:
  using discard = int[];
  (void)discard{0, ((retval.emplace_back(std::forward<Ts>(ts))), void(), 0)...};
  return retval;  // NRVO!
}
// the empty overload:
template <class T>
std::vector<T> MakeVector() {
  return {};
}

template <class T>
std::vector<std::reference_wrapper<T>> ReferenceVector(
    std::vector<std::unique_ptr<T>>& vec) {
  std::vector<std::reference_wrapper<T>> retval;
  retval.reserve(vec.size());
  for (auto& ptr : vec) {
    retval.push_back(*ptr);
  }
  return retval;
}

template <class T>
std::vector<std::reference_wrapper<const T>> ImmutableReferenceVector(
    const std::vector<std::unique_ptr<T>>& vec) {
  std::vector<std::reference_wrapper<const T>> retval;
  retval.reserve(vec.size());
  for (auto& ptr : vec) {
    retval.push_back(*ptr);
  }
  return retval;
}

}  // namespace kush::util
