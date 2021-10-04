#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value/value.h"
#include "util/vector_util.h"

namespace kush::compile {

SchemaValues::SchemaValues(int num_values) : values_(num_values) {}

void SchemaValues::ResetValues() { values_.clear(); }

void SchemaValues::AddVariable(std::unique_ptr<proxy::IRValue> value) {
  values_.push_back(std::move(value));
}

const proxy::IRValue& SchemaValues::Value(int idx) const {
  return *values_[idx];
}

std::vector<std::reference_wrapper<const proxy::IRValue>> SchemaValues::Values()
    const {
  return util::ImmutableReferenceVector(values_);
}

std::vector<std::reference_wrapper<proxy::IRValue>> SchemaValues::Values() {
  return util::ReferenceVector(values_);
}

void SchemaValues::SetValues(
    std::vector<std::unique_ptr<proxy::IRValue>> values) {
  values_ = std::move(values);
}

void SchemaValues::SetValue(int idx, std::unique_ptr<proxy::IRValue> value) {
  values_[idx] = std::move(value);
}

}  // namespace kush::compile
