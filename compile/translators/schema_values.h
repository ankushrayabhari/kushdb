#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value.h"

namespace kush::compile {

class SchemaValues {
 public:
  SchemaValues() = default;
  void AddVariable(std::unique_ptr<proxy::Value> value);
  const proxy::Value& Value(int idx) const;
  std::vector<std::reference_wrapper<const proxy::Value>> Values() const;

 private:
  std::vector<std::unique_ptr<proxy::Value>> values_;
};

}  // namespace kush::compile
