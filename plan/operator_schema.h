#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"

namespace kush::plan {

class OperatorSchema {
 public:
  class Column {
   public:
    Column(std::string_view name, catalog::SqlType type);
    std::string_view Name() const;
    catalog::SqlType Type() const;

   private:
    std::string name_;
    catalog::SqlType type_;
  };

  const std::vector<Column>& Columns() const;
  const Column& GetColumn(int idx) const;
  int GetColumnIndex(std::string_view name) const;
  nlohmann::json ToJson() const;
  void AddColumn(std::string_view name, catalog::SqlType type);

 private:
  std::unordered_map<std::string, int> column_name_to_idx_;
  std::vector<Column> columns_;
};

}  // namespace kush::plan
