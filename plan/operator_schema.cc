#include "plan/operator_schema.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "catalog/sql_type.h"
#include "magic_enum.hpp"

namespace kush::plan {

OperatorSchema::Column::Column(std::string_view name, catalog::SqlType type)
    : name_(name), type_(type) {}

std::string_view OperatorSchema::Column::Name() const { return name_; }

catalog::SqlType OperatorSchema::Column::Type() const { return type_; }

void OperatorSchema::AddColumn(std::string_view name, catalog::SqlType type) {
  column_name_to_idx_[std::string(name)] = columns_.size();
  columns_.emplace_back(name, type);
}

const std::vector<OperatorSchema::Column>& OperatorSchema::Columns() const {
  return columns_;
}

const OperatorSchema::Column& OperatorSchema::GetColumn(int idx) const {
  return columns_[idx];
}

int OperatorSchema::GetColumnIndex(std::string_view name) const {
  return column_name_to_idx_.at(std::string(name));
}

nlohmann::json OperatorSchema::ToJson() const {
  nlohmann::json j;
  for (const Column& c : columns_) {
    nlohmann::json c_json;
    c_json["name"] = std::string(c.Name());
    c_json["type"] = magic_enum::enum_name(c.Type());
    j["columns"].push_back(c_json);
  }
  return j;
}

}  // namespace kush::plan
