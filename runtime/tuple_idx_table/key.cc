#include "runtime/tuple_idx_table/key.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace runtime::TupleIdxTable {

Key::Key(std::unique_ptr<int32_t[]> data, int32_t len)
    : data_(std::move(data)), len_(len) {}

std::unique_ptr<Key> Key::CreateKey(int32_t* data, int32_t len) {
  auto copy = std::unique_ptr<int32_t[]>(new int32_t[len]);
  memcpy(copy.get(), data, len * sizeof(int32_t));
  return std::make_unique<Key>(std::move(copy), len);
}

bool Key::operator>(const Key& k) const {
  if (len_ != k.len_) {
    throw std::runtime_error("mismatched lengths");
  }

  for (int i = 0; i < len_; i++) {
    if (data_[i] > k.data_[i]) {
      return true;
    } else if (data_[i] < k.data_[i]) {
      return false;
    }
  }

  return false;
}

bool Key::operator>=(const Key& k) const {
  if (len_ != k.len_) {
    throw std::runtime_error("mismatched lengths");
  }

  for (int i = 0; i < len_; i++) {
    if (data_[i] > k.data_[i]) {
      return true;
    } else if (data_[i] < k.data_[i]) {
      return false;
    }
  }

  return true;
}

bool Key::operator<(const Key& k) const {
  if (len_ != k.len_) {
    throw std::runtime_error("mismatched lengths");
  }

  for (int i = 0; i < len_; i++) {
    if (data_[i] < k.data_[i]) {
      return true;
    } else if (data_[i] > k.data_[i]) {
      return false;
    }
  }

  return false;
}

bool Key::operator==(const Key& k) const {
  if (len_ != k.len_) {
    throw std::runtime_error("mismatched lengths");
  }

  for (int i = 0; i < len_; i++) {
    if (data_[i] != k.data_[i]) {
      return false;
    }
  }

  return true;
}

int32_t& Key::operator[](std::size_t i) { return data_[i]; }

const int32_t& Key::operator[](std::size_t i) const { return data_[i]; }

}  // namespace runtime::TupleIdxTable