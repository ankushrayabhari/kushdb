#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value.h"

namespace kush::compile {

template <typename T>
class SchemaValues {
 public:
  SchemaValues(int num_values);

  void ResetValues();
  void AddVariable(std::unique_ptr<proxy::Value<T>> value);

  const proxy::Value<T>& Value(int idx) const;
  std::vector<std::reference_wrapper<const proxy::Value<T>>> Values() const;
  std::vector<std::reference_wrapper<proxy::Value<T>>> Values();

  void SetValues(std::vector<std::unique_ptr<proxy::Value<T>>> values);
  void SetValue(int idx, std::unique_ptr<proxy::Value<T>> value);

 private:
  std::vector<std::unique_ptr<proxy::Value<T>>> values_;
};

}  // namespace kush::compile
