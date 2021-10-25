#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace kush::parse {

class Table {};

class BaseTable : public Table {
 public:
  BaseTable(std::string_view name, std::string_view alias);

 private:
  std::string name_;
  std::string alias_;
};

class CrossProductTable : public Table {
 public:
  CrossProductTable(std::vector<std::unique_ptr<Table>> children);

 private:
  std::vector<std::unique_ptr<Table>> children_;
};

}  // namespace kush::parse