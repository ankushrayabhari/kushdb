#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/value.h"

namespace kush::compile {

template <typename T>
void SchemaValues<T>::AddVariable(const proxy::Value<T>& value) {
  values_.push_back(value);
}

template <typename T>
const proxy::Value<T>& SchemaValues<T>::Value(int idx) const {
  return values_[idx];
}

template <typename T>
const std::vector<std::reference_wrapper<const proxy::Value<T>>>&
SchemaValues<T>::Values() const {
  return values_;
}

INSTANTIATE_ON_IR(SchemaValues);

}  // namespace kush::compile
