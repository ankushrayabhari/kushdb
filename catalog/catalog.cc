#include "catalog/catalog.h"

#include <memory>
#include <string>

#include "catalog/sql_type.h"

namespace kush::catalog {

Column::Column(std::string_view n, SqlType t, std::string_view p)
    : name_(n), type_(t), path_(p) {}

std::string_view Column::Name() const { return name_; }

SqlType Column::Type() const { return type_; }

std::string_view Column::Path() const { return path_; }

Table::Table(std::string_view name) : name_(name) {}

Column& Table::Insert(std::string_view attr, SqlType type,
                      std::string_view path) {
  name_to_col_.insert({std::string(attr), Column(attr, type, path)});
  return name_to_col_.at(attr);
}

const Column& Table::operator[](std::string_view attr) const {
  return name_to_col_.at(attr);
}

std::vector<std::reference_wrapper<const Column>> Table::Columns() const {
  std::vector<std::reference_wrapper<const Column>> output;
  for (const auto& [name, col] : name_to_col_) {
    output.push_back(col);
  }
  return output;
}

Table& Database::Insert(std::string_view table) {
  name_to_table_.insert({std::string(table), Table(table)});
  return name_to_table_.at(table);
}

const Table& Database::operator[](std::string_view table) const {
  return name_to_table_.at(table);
}

bool Database::Contains(std::string_view table) const {
  return name_to_table_.find(table) != name_to_table_.end();
}

}  // namespace kush::catalog