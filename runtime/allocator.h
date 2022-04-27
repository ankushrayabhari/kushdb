#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace kush::runtime {

class Allocator final {
 public:
  Allocator();
  virtual ~Allocator();
  uint8_t* Allocate(std::size_t s);

  template <typename T, class... Args>
  T* Allocate(Args&&... args) {
    auto dest = Allocate(sizeof(T));
    return new (dest) T(std::forward<Args>(args)...);
  }

 private:
  std::vector<uint8_t*> pages_;
  std::size_t data_offset_;
};

}  // namespace kush::runtime