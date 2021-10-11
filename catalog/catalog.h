#pragma once

#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"

#include "catalog/sql_type.h"

namespace kush::catalog {

class Column {
 public:
  Column(std::string_view name, SqlType type, std::string_view path,
         std::string_view null_path, std::string_view index_path);
  std::string_view Name() const;
  SqlType Type() const;
  std::string_view Path() const;

  std::string_view NullPath() const;
  bool Nullable() const;

  std::string_view IndexPath() const;
  bool HasIndex() const;

 private:
  const std::string name_;
  const SqlType type_;
  const std::string path_;
  const std::string null_path_;
  const std::string index_path_;
};

class Table {
 public:
  explicit Table(std::string_view name);

  std::string_view Name() const;
  Column& Insert(std::string_view attr, SqlType type, std::string_view path,
                 std::string_view null_path, std::string_view index_path);
  const Column& operator[](std::string_view attr) const;
  std::vector<std::reference_wrapper<const Column>> Columns() const;

 private:
  const std::string name_;
  absl::flat_hash_map<std::string, Column> name_to_col_;
};

class Database {
 public:
  Table& Insert(std::string_view table);
  const Table& operator[](std::string_view table) const;
  bool Contains(std::string_view table) const;

 private:
  absl::flat_hash_map<std::string, Table> name_to_table_;
};

}  // namespace kush::catalog