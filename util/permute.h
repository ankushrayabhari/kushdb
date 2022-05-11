#pragma once

#include <cstdint>

namespace kush::util {

struct PermutationTable {
 public:
  static PermutationTable& Get();

 private:
  constexpr PermutationTable();

 public:
  PermutationTable(PermutationTable const&) = delete;
  void operator=(PermutationTable const&) = delete;

  uint32_t* Addr();

 private:
  alignas(32) uint32_t values[256][8] = {};
};

}  // namespace kush::util