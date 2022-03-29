#pragma once

#include <cstdint>
#include <memory>

namespace runtime::TupleIdxTable {

class Key {
 public:
  Key(std::unique_ptr<int32_t[]> data, int32_t len);

  static std::unique_ptr<Key> CreateKey(int32_t *tuple_idx_array, int32_t len);

  int32_t &operator[](std::size_t i);
  const int32_t &operator[](std::size_t i) const;

  bool operator>(const Key &k) const;
  bool operator<(const Key &k) const;
  bool operator>=(const Key &k) const;
  bool operator==(const Key &k) const;

 private:
  std::unique_ptr<int32_t[]> data_;
  int32_t len_;
};

}  // namespace runtime::TupleIdxTable