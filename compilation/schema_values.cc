#include "compilation/schema_values.h"

#include <string>
#include <vector>

namespace kush::compile {

SchemaValues::SchemaValues(std::vector<std::string> values)
    : values_(std::move(values)) {}

std::string SchemaValues::Get(int idx) { return values_[idx]; }

}  // namespace kush::compile
