#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/value.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
SchemaValues<T>::SchemaValues(int num_values) : values_(num_values) {}

template <typename T>
void SchemaValues<T>::ResetValues() {
  values_.clear();
}

template <typename T>
void SchemaValues<T>::AddVariable(std::unique_ptr<proxy::Value<T>> value) {
  values_.push_back(std::move(value));
}

template <typename T>
const proxy::Value<T>& SchemaValues<T>::Value(int idx) const {
  return *values_[idx];
}

template <typename T>
std::vector<std::reference_wrapper<const proxy::Value<T>>>
SchemaValues<T>::Values() const {
  return util::ImmutableReferenceVector(values_);
}

template <typename T>
std::vector<std::reference_wrapper<proxy::Value<T>>> SchemaValues<T>::Values() {
  return util::ReferenceVector(values_);
}

template <typename T>
void SchemaValues<T>::SetValues(
    std::vector<std::unique_ptr<proxy::Value<T>>> values) {
  values_ = std::move(values);
}

template <typename T>
void SchemaValues<T>::SetValue(int idx,
                               std::unique_ptr<proxy::Value<T>> value) {
  values_[idx] = std::move(value);
}

INSTANTIATE_ON_IR(SchemaValues);

}  // namespace kush::compile
