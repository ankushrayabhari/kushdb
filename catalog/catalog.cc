#include "catalog/catalog.h"

#include <memory>
#include <string>

#include "data/column_data.h"

namespace kush {
namespace catalog {

Column::Column(const std::string& n, const std::string& t, const std::string& p)
    : name(n), type(t), path(p) {}

Table::Table(const std::string& n) : name(n) {}

Column& Table::insert(const std::string& attr, const std::string& type,
                      const std::string& path) {
  name_to_col_.insert({attr, Column(attr, type, path)});
  return name_to_col_.at(attr);
}

const Column& Table::operator[](const std::string& attr) const {
  return name_to_col_.at(attr);
}

bool Table::contains(const std::string& attr) const {
  return name_to_col_.find(attr) != name_to_col_.end();
}

Table& Database::insert(const std::string& table) {
  name_to_table_.insert({table, Table(table)});
  return name_to_table_.at(table);
}

const Table& Database::operator[](const std::string& table) const {
  return name_to_table_.at(table);
}

bool Database::contains(const std::string& table) const {
  return name_to_table_.find(table) != name_to_table_.end();
}

const std::unordered_map<std::string, Column>& Table::Columns() const {
  return name_to_col_;
}

}  // namespace catalog
}  // namespace kush