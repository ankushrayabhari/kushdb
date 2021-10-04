#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value/sql_value.h"
#include "util/vector_util.h"

namespace kush::compile {

void SchemaValues::ResetValues() { values_.clear(); }

void SchemaValues::AddVariable(proxy::SQLValue value) {
  values_.push_back(std::move(value));
}

const proxy::SQLValue& SchemaValues::Value(int idx) const {
  return values_[idx];
}

std::vector<std::reference_wrapper<const proxy::SQLValue>>
SchemaValues::Values() const {
  return util::ImmutableReferenceVector(values_);
}

std::vector<std::reference_wrapper<proxy::SQLValue>> SchemaValues::Values() {
  return util::ReferenceVector(values_);
}

void SchemaValues::SetValues(std::vector<proxy::SQLValue> values) {
  values_ = std::move(values);
}

void SchemaValues::SetValue(int idx, proxy::SQLValue value) {
  values_[idx] = std::move(value);
}

}  // namespace kush::compile
