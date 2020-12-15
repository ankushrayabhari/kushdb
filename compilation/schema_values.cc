#include "compilation/schema_values.h"

#include <string>
#include <vector>

namespace kush::compile {

void SchemaValues::AddVariable(std::string variable, std::string type) {
  values_.emplace_back(std::move(variable), std::move(type));
}

std::string SchemaValues::Variable(int idx) const { return values_[idx].first; }

std::string SchemaValues::Type(int idx) const { return values_[idx].second; }

}  // namespace kush::compile
