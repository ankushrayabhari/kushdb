#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace kush::parse {

class Table {
 public:
  virtual ~Table() = default;
};

class BaseTable : public Table {
 public:
  BaseTable(std::string_view name, std::string_view alias);

  std::string_view Name() const;
  std::string_view Alias() const;

 private:
  std::string name_;
  std::string alias_;
};

class CrossProductTable : public Table {
 public:
  CrossProductTable(std::vector<std::unique_ptr<Table>> children);

  std::vector<std::reference_wrapper<const Table>> Children() const;

 private:
  std::vector<std::unique_ptr<Table>> children_;
};

}  // namespace kush::parse