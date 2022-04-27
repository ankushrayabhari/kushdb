#pragma once

#include <cstdint>
#include <memory>

#include "runtime/allocator.h"

namespace kush::runtime::TupleIdxTable {

class Key {
 public:
  Key(uint8_t* data, int32_t len);

  static Key* CreateKey(Allocator& allocator, const int32_t* tuple_idx_array,
                        int32_t len);

  uint8_t& operator[](std::size_t i);
  const uint8_t& operator[](std::size_t i) const;

  bool operator>(const Key& k) const;
  bool operator<(const Key& k) const;
  bool operator>=(const Key& k) const;
  bool operator==(const Key& k) const;

  int32_t* Data() const;
  int32_t Length() const;

 private:
  uint8_t* data_;
  int32_t len_;
};

}  // namespace kush::runtime::TupleIdxTable