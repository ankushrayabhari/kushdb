#include "catalog/catalog.h"

#include <memory>
#include <string>

#include "catalog/sql_type.h"

namespace kush::catalog {

Column::Column(std::string_view n, const Type& t, std::string_view p,
               std::string_view np, std::string_view ip)
    : name_(n), type_(t), path_(p), null_path_(np), index_path_(ip) {}

std::string_view Column::Name() const { return name_; }

const Type& Column::GetType() const { return type_; }

std::string_view Column::Path() const { return path_; }

std::string_view Column::NullPath() const { return null_path_; }

bool Column::Nullable() const { return !null_path_.empty(); }

std::string_view Column::IndexPath() const { return index_path_; }

bool Column::HasIndex() const { return !index_path_.empty(); }

Table::Table(std::string_view name) : name_(name) {}

Column& Table::Insert(std::string_view attr, const Type& type,
                      std::string_view path, std::string_view null_path,
                      std::string_view index_path) {
  name_to_col_.insert(
      {std::string(attr), Column(attr, type, path, null_path, index_path)});
  return name_to_col_.at(attr);
}

std::string_view Table::Name() const { return name_; }

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