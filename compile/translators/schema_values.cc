#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/value.h"
#include "util/vector_util.h"

namespace kush::compile {

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
void SchemaValues<T>::SetValues(
    std::vector<std::unique_ptr<proxy::Value<T>>> values) {
  values_ = std::move(values);
}

INSTANTIATE_ON_IR(SchemaValues);

}  // namespace kush::compile
