#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value.h"

namespace kush::compile {

class SchemaValues {
 public:
  SchemaValues(int num_values);

  void ResetValues();
  void AddVariable(std::unique_ptr<proxy::Value> value);

  const proxy::Value& Value(int idx) const;
  std::vector<std::reference_wrapper<const proxy::Value>> Values() const;
  std::vector<std::reference_wrapper<proxy::Value>> Values();

  void SetValues(std::vector<std::unique_ptr<proxy::Value>> values);
  void SetValue(int idx, std::unique_ptr<proxy::Value> value);

 private:
  std::vector<std::unique_ptr<proxy::Value>> values_;
};

}  // namespace kush::compile
