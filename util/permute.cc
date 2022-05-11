#include "util/permute.h"

namespace kush::util {

PermutationTable& PermutationTable::Get() {
  static PermutationTable instance;
  return instance;
}

constexpr PermutationTable::PermutationTable() {
  for (int i = 0; i < 256; i++) {
    // set it to 0
    for (int j = 0; j < 8; j++) {
      values[i][j] = 0;
    }

    int next = 0;
    for (int bit = 0; bit < 8; bit++) {
      if (i & (1 << bit)) {
        values[i][next++] = bit;
      }
    }
  }
}

uint32_t* PermutationTable::GetEntry(uint32_t i) { return &values[i][0]; }

uint32_t* PermutationTable::Addr() { return &values[0][0]; }

}  // namespace kush::util