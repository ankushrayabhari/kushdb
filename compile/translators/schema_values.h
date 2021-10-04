#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value/value.h"

namespace kush::compile {

class SchemaValues {
 public:
  SchemaValues(int num_values);

  void ResetValues();
  void AddVariable(std::unique_ptr<proxy::IRValue> value);

  const proxy::IRValue& Value(int idx) const;
  std::vector<std::reference_wrapper<const proxy::IRValue>> Values() const;
  std::vector<std::reference_wrapper<proxy::IRValue>> Values();

  void SetValues(std::vector<std::unique_ptr<proxy::IRValue>> values);
  void SetValue(int idx, std::unique_ptr<proxy::IRValue> value);

 private:
  std::vector<std::unique_ptr<proxy::IRValue>> values_;
};

}  // namespace kush::compile
