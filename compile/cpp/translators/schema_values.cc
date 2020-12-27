#include "compile/cpp/translators/schema_values.h"

#include <string>
#include <vector>

namespace kush::compile {

void SchemaValues::AddVariable(std::string variable, std::string type) {
  values_.emplace_back(std::move(variable), std::move(type));
}

std::string SchemaValues::Variable(int idx) const { return values_[idx].first; }

std::string SchemaValues::Type(int idx) const { return values_[idx].second; }

const std::vector<std::pair<std::string, std::string>>& SchemaValues::Values()
    const {
  return values_;
}

}  // namespace kush::compile
