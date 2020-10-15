#include "catalog/catalog.h"

#include <memory>
#include <string>

#include "data/column_data.h"

namespace skinner {

Column::Column(const std::string& n, const std::string& t) : name(n), type(t) {}

Column::Column(const std::string& n, const std::string& t, const std::string& p)
    : data_(p), name(n), type(t) {}

Column& Table::insert(const std::string& attr, const std::string& type) {
  name_to_col_.emplace(attr, std::make_unique<Column>(attr, type));
  return *name_to_col_[attr];
}

Column& Table::insert(const std::string& attr, const std::string& type,
                      const std::string& path) {
  name_to_col_.emplace(attr, std::make_unique<Column>(attr, type, path));
  return *name_to_col_[attr];
}

Column& Table::operator[](const std::string& attr) {
  return *name_to_col_[attr];
}

bool Table::contains(const std::string& attr) {
  return name_to_col_.find(attr) != name_to_col_.end();
}

}  // namespace skinner