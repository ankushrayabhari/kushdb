#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "catalog/sql_type.h"

namespace kush::catalog {

class Column {
 public:
  Column(const std::string& n, SqlType t, const std::string& p);

  const std::string name;
  const SqlType type;
  const std::string path;
};

class Table {
  std::unordered_map<std::string, Column> name_to_col_;

 public:
  Table(const std::string& n);
  const std::string name;

  Column& insert(const std::string& attr, SqlType type,
                 const std::string& path);
  const Column& operator[](const std::string& attr) const;
  bool contains(const std::string& attr) const;
  const std::unordered_map<std::string, Column>& Columns() const;
};

class Database {
  std::unordered_map<std::string, Table> name_to_table_;

 public:
  Table& insert(const std::string& table);
  const Table& operator[](const std::string& table) const;
  bool contains(const std::string& table) const;
};

}  // namespace kush::catalog