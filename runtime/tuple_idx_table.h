#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace std {
template <>
struct hash<std::vector<int32_t>> {
  size_t operator()(const std::vector<int32_t>& x) const {
    std::size_t seed(0);
    for (const int v : x) {
      seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}  // namespace std

namespace kush::runtime::TupleIdxTable {

std::unordered_set<std::vector<int32_t>>* Create();

void Insert(std::unordered_set<std::vector<int32_t>>* ht, int32_t* arr,
            int32_t n);

int32_t Size(std::unordered_set<std::vector<int32_t>>* ht);

void Begin(std::unordered_set<std::vector<int32_t>>* ht,
           std::unordered_set<std::vector<int32_t>>::const_iterator** it);

void Increment(std::unordered_set<std::vector<int32_t>>::const_iterator** it);

int32_t* Get(std::unordered_set<std::vector<int32_t>>::const_iterator** it);

void FreeIt(std::unordered_set<std::vector<int32_t>>::const_iterator** it);

void Free(std::unordered_set<std::vector<int32_t>>* ht);

}  // namespace kush::runtime::TupleIdxTable
