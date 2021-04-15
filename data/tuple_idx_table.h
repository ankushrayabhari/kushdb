#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace std {
template <>
struct hash<std::vector<uint32_t>> {
  size_t operator()(const std::vector<uint32_t>& x) const {
    std::size_t seed(0);
    for (const int v : x) {
      seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}  // namespace std

namespace kush::data {

std::unordered_set<std::vector<uint32_t>>* CreateTupleIdxTable();

void Insert(std::unordered_set<std::vector<uint32_t>>* ht, uint32_t* arr,
            uint32_t n);

uint32_t Size(std::unordered_set<std::vector<uint32_t>>* ht);

void Begin(std::unordered_set<std::vector<uint32_t>>* ht,
           std::unordered_set<std::vector<uint32_t>>::const_iterator** it);

void Increment(std::unordered_set<std::vector<uint32_t>>::const_iterator** it);

uint32_t* Get(std::unordered_set<std::vector<uint32_t>>::const_iterator** it);

void Free(std::unordered_set<std::vector<uint32_t>>::const_iterator** it);

void Free(std::unordered_set<std::vector<uint32_t>>* ht);

}  // namespace kush::data
