#include "compile/translators/schema_values.h"

#include <string>
#include <vector>

#include "compile/proxy/value.h"
#include "util/vector_util.h"

namespace kush::compile {

void SchemaValues::AddVariable(std::unique_ptr<proxy::Value> value) {
  values_.push_back(std::move(value));
}

const proxy::Value& SchemaValues::Value(int idx) const { return *values_[idx]; }

std::vector<std::reference_wrapper<const proxy::Value>> SchemaValues::Values()
    const {
  return util::ImmutableReferenceVector(values_);
}

}  // namespace kush::compile
