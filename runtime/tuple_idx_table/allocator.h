#pragma once

#include <cstdint>
#include <vector>

namespace kush::runtime::TupleIdxTable {

class Allocator {
 public:
  Allocator();
  uint8_t* AllocateData(std::size_t s);
  void Free();

 private:
  std::vector<uint8_t*> pages_;
  std::size_t data_offset_;
};

}  // namespace kush::runtime::TupleIdxTable