#include "runtime/tuple_idx_table/key.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace kush::runtime::TupleIdxTable {

Key::Key(uint8_t* data, int32_t len) : data_(data), len_(len) {}

Key* Key::CreateKey(Allocator& allocator, const int32_t* data, int32_t len) {
  auto copy = allocator.AllocateData(len * sizeof(int32_t));
  memcpy(copy, data, len * sizeof(int32_t));

  auto dest = allocator.AllocateData(sizeof(Key));
  return new (dest) Key(copy, len * sizeof(int32_t));
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

int32_t Key::Length() const { return len_; }

int32_t* Key::Data() const { return (int32_t*)data_; }

}  // namespace kush::runtime::TupleIdxTable