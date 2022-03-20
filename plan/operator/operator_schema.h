#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"

namespace kush::plan {

class OperatorSchemaProvider;

class OperatorSchema {
 public:
  class Column {
   public:
    Column(std::string_view name, std::unique_ptr<Expression> expr);
    std::string_view Name() const;

    Expression& MutableExpr();
    const Expression& Expr() const;

   private:
    std::string name_;
    std::unique_ptr<Expression> expr_;
  };

  std::vector<Column>& MutableColumns();
  const std::vector<Column>& Columns() const;
  void RemoveColumn(int idx);

  int GetColumnIndex(std::string_view name) const;
  nlohmann::json ToJson() const;
  void AddDerivedColumn(std::string_view name,
                        std::unique_ptr<Expression> expr);
  void AddPassthroughColumns(const OperatorSchemaProvider& op,
                             int child_idx = 0);
  void AddPassthroughColumns(const OperatorSchemaProvider& op,
                             const std::vector<std::string>& columns,
                             int child_idx = 0);
  void AddPassthroughColumn(const OperatorSchemaProvider& op,
                            std::string_view base_name,
                            std::string_view derived_name, int child_idx = 0);
  void AddVirtualPassthroughColumns(const OperatorSchema& schema,
                                    const std::vector<std::string>& columns);

  void AddGeneratedColumns(const kush::catalog::Table& table);
  void AddGeneratedColumns(const kush::catalog::Table& table,
                           const std::vector<std::string>& columns);

 private:
  absl::flat_hash_map<std::string, int> column_name_to_idx_;
  std::vector<Column> columns_;
};

class OperatorSchemaProvider {
 public:
  virtual const OperatorSchema& Schema() const = 0;
  virtual OperatorSchema& MutableSchema() = 0;
};

}  // namespace kush::plan
