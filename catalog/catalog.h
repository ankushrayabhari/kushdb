#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "catalog/catalog.h"
#include "data/column_data.h"

namespace skinner {

class Column {
  ColumnData<void*> data_;

 public:
  Column(const std::string& n, const std::string& t);
  Column(const std::string& n, const std::string& t, const std::string& p);
  Column(const Column&) = delete;
  Column& operator=(const Column&) = delete;

  const std::string name;
  const std::string type;

  template <typename T>
  ColumnData<T>& Data() {
    return reinterpret_cast<ColumnData<T>&>(data_);
  }
};

class Table {
  std::unordered_map<std::string, std::unique_ptr<Column>> name_to_col_;

 public:
  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;

  const std::string name;

  Column& insert(const std::string& attr, const std::string& type);
  Column& insert(const std::string& attr, const std::string& type,
                 const std::string& path);
  Column& operator[](const std::string& attr);
  bool contains(const std::string& attr);
};

}  // namespace skinner