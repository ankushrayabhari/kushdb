#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "catalog/sql_type.h"
#include "expression/expression.h"
#include "nlohmann/json.hpp"

namespace kush::plan {

class OperatorSchema {
 public:
  class Column {
   public:
    Column(std::string_view name, std::unique_ptr<Expression> expr);
    std::string_view Name() const;
    Expression& Expr() const;

   private:
    std::string name_;
    std::unique_ptr<Expression> expr_;
  };

  const std::vector<Column>& Columns() const;
  int GetColumnIndex(std::string_view name) const;
  nlohmann::json ToJson() const;
  void AddDerivedColumn(std::string_view name,
                        std::unique_ptr<Expression> expr);
  void AddGeneratedColumn(std::string_view name, catalog::SqlType type);

 private:
  absl::flat_hash_map<std::string, int> column_name_to_idx_;
  std::vector<Column> columns_;
};

}  // namespace kush::plan
