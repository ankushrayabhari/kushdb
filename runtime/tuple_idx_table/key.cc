#include "runtime/tuple_idx_table/key.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace runtime::TupleIdxTable {

Key::Key(std::unique_ptr<uint8_t[]> data, int32_t len)
    : data_(std::move(data)), len_(len) {}

std::unique_ptr<Key> Key::CreateKey(int32_t* data, int32_t len) {
  auto copy = std::unique_ptr<uint8_t[]>(new uint8_t[len * sizeof(int32_t)]);
  memcpy(copy.get(), data, len * sizeof(int32_t));
  return std::make_unique<Key>(std::move(copy), len);
}

bool Key::operator>(const Key& k) const {
  for (int i = 0; i < std::min(len_, k.len_); i++) {
    if (data_[i] > k.data_[i]) {
      return true;
    } else if (data_[i] < k.data_[i]) {
      return false;
    }
  }

  return len_ > k.len_;
}

bool Key::operator>=(const Key& k) const {
  for (int i = 0; i < std::min(len_, k.len_); i++) {
    if (data_[i] > k.data_[i]) {
      return true;
    } else if (data_[i] < k.data_[i]) {
      return false;
    }
  }

  return len_ >= k.len_;
}

bool Key::operator<(const Key& k) const {
  for (int i = 0; i < std::min(len_, k.len_); i++) {
    if (data_[i] < k.data_[i]) {
      return true;
    } else if (data_[i] > k.data_[i]) {
      return false;
    }
  }

  return len_ < k.len_;
}

bool Key::operator==(const Key& k) const {
  if (len_ != k.len_) {
    return false;
  }

  for (int i = 0; i < len_; i++) {
    if (data_[i] != k.data_[i]) {
      return false;
    }
  }

  return true;
}

uint8_t& Key::operator[](std::size_t i) { return data_[i]; }

const uint8_t& Key::operator[](std::size_t i) const { return data_[i]; }

}  // namespace runtime::TupleIdxTable