#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/cpp/proxy/value.h"

namespace kush::compile::cpp {

class SchemaValues {
 public:
  SchemaValues() = default;
  void AddVariable(std::unique_ptr<proxy::Value> value);
  const proxy::Value& Value(int idx) const;
  std::vector<std::reference_wrapper<const proxy::Value>> Values() const;

 private:
  std::vector<std::unique_ptr<proxy::Value>> values_;
};

}  // namespace kush::compile::cpp
