#include "runtime/tuple_idx_table/allocator.h"

#include <cstdint>
#include <vector>

namespace kush::runtime::TupleIdxTable {

constexpr std::size_t PAGE_SIZE = 1 << 16;

Allocator::Allocator() : data_offset_(PAGE_SIZE) {}

uint8_t* Allocator::AllocateData(std::size_t s) {
  if (data_offset_ + s > PAGE_SIZE) {
    pages_.push_back(new uint8_t[PAGE_SIZE]);
    data_offset_ = 0;
  }

  auto ret = pages_.back() + data_offset_;
  data_offset_ += s;
  return ret;
}

void Allocator::Free() {
  for (auto page : pages_) {
    delete[] page;
  }
}

}  // namespace kush::runtime::TupleIdxTable