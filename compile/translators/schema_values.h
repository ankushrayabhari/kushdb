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

  void AddVariable(const proxy::Value<T>& value);
  const proxy::Value<T>& Value(int idx) const;
  const std::vector<std::reference_wrapper<const proxy::Value<T>>>& Values()
      const;

 private:
  std::vector<std::reference_wrapper<const proxy::Value<T>>> values_;
};

}  // namespace kush::compile
