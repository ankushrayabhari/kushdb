#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value.h"

namespace kush::compile {

template <typename T>
class SchemaValues {
 public:
  SchemaValues() = default;

  void AddVariable(const proxy::Value<T>& value) { values_.push_back(value); }

  const proxy::Value<T>& Value(int idx) const { return values_[idx]; }

  const std::vector<std::reference_wrapper<const proxy::Value<T>>>& Values()
      const {
    return values_;
  }

 private:
  std::vector<std::reference_wrapper<proxy::Value<T>>> values_;
};

}  // namespace kush::compile
