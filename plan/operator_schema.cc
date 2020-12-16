#include "plan/operator_schema.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "catalog/sql_type.h"
#include "expression/expression.h"
#include "magic_enum.hpp"
#include "nlohmann/json.hpp"

namespace kush::plan {

OperatorSchema::Column::Column(std::string_view name, catalog::SqlType type,
                               std::unique_ptr<Expression> expr)
    : name_(name), type_(type), expr_(std::move(expr)) {}

std::string_view OperatorSchema::Column::Name() const { return name_; }

catalog::SqlType OperatorSchema::Column::Type() const { return type_; }

Expression& OperatorSchema::Column::Expr() const {
  if (expr_ == nullptr) {
    throw std::runtime_error("Generated column has no derived expression");
  }
  return *expr_;
}

void OperatorSchema::AddDerivedColumn(std::string_view name,
                                      catalog::SqlType type,
                                      std::unique_ptr<Expression> expr) {
  column_name_to_idx_[std::string(name)] = columns_.size();
  columns_.emplace_back(name, type, std::move(expr));
}

void OperatorSchema::AddGeneratedColumn(std::string_view name,
                                        catalog::SqlType type) {
  int idx = columns_.size();
  column_name_to_idx_[std::string(name)] = idx;
  columns_.emplace_back(name, type, nullptr);
}

const std::vector<OperatorSchema::Column>& OperatorSchema::Columns() const {
  return columns_;
}

int OperatorSchema::GetColumnIndex(std::string_view name) const {
  return column_name_to_idx_.at(std::string(name));
}

nlohmann::json OperatorSchema::ToJson() const {
  nlohmann::json j;
  for (const Column& c : columns_) {
    nlohmann::json c_json;
    c_json["name"] = std::string(c.Name());
    c_json["value"] = c.Expr().ToJson();
    c_json["type"] = magic_enum::enum_name(c.Type());
    j["columns"].push_back(c_json);
  }
  return j;
}

}  // namespace kush::plan
