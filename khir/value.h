#pragma once

#include <cstdint>

namespace kush::khir {

class Value {
 public:
  Value();
  explicit Value(uint32_t idx);
  Value(uint32_t idx, bool constant_global);
  uint32_t Serialize() const;
  uint32_t GetIdx() const;
  bool IsConstantGlobal() const;
  bool operator==(const Value& rhs) const;

 private:
  uint32_t idx_;
};

}  // namespace kush::khir