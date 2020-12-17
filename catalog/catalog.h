#pragma once
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "catalog/sql_type.h"

namespace kush::catalog {

class Column {
 public:
  Column(std::string_view n, SqlType t, std::string_view p);
  std::string_view Name() const;
  SqlType Type() const;
  std::string_view Path() const;

 private:
  const std::string name_;
  const SqlType type_;
  const std::string path_;
};

class Table {
  absl::flat_hash_map<std::string, Column> name_to_col_;

 public:
  Table(std::string_view n);
  const std::string name;

  Column& insert(std::string_view attr, SqlType type, std::string_view path);
  const Column& operator[](std::string_view attr) const;
  bool contains(std::string_view attr) const;
};

class Database {
  absl::flat_hash_map<std::string, Table> name_to_table_;

 public:
  Table& insert(std::string_view table);
  const Table& operator[](std::string_view table) const;
  bool contains(std::string_view table) const;
};

}  // namespace kush::catalog