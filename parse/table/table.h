#pragma once
#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/statement.h"

namespace kush::parse {

class Table {};

class BaseTable : public Table {
 public:
  BaseTable(std::string_view table_name, std::string_view alias);
};

class CrossProductTable : public Table {
 public:
  CrossProductTable(std::vector<std::unique_ptr<Table>> children);
};

}  // namespace kush::parse