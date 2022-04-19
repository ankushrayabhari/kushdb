#include "khir/value.h"

#include <exception>
#include <stdexcept>

namespace kush::khir {

Value::Value() : idx_(UINT32_MAX) {}

Value::Value(uint32_t idx) : idx_(idx) {}

Value::Value(uint32_t idx, bool constant_global) : idx_(idx) {
  if (idx_ > 0x7FFFFF) {
    throw std::runtime_error("Invalid idx");
  }

  if (constant_global) {
    idx_ = idx_ | (1 << 23);
  }
}

uint32_t Value::Serialize() const { return idx_; }

uint32_t Value::GetIdx() const { return idx_ & 0x7FFFFF; }

bool Value::IsConstantGlobal() const { return (idx_ & (1 << 23)) != 0; }

bool Value::operator==(const Value& rhs) const { return idx_ == rhs.idx_; }

}  // namespace kush::khir