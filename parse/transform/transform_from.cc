#include "parse/transform/transform_from.h"

#include <memory>
#include <vector>

#include "parse/table/table.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::string TransformAlias(duckdb_libpgquery::PGAlias* root) {
  if (root == nullptr) {
    return "";
  }

  if (root->colnames) {
    throw std::runtime_error("Column aliases not supported.");
  }

  return root->aliasname;
}

std::unique_ptr<BaseTable> TransformBaseTable(
    duckdb_libpgquery::PGRangeVar& base_table) {
  if (base_table.catalogname != nullptr) {
    throw std::runtime_error("Database specifier not supported.");
  }
  if (base_table.schemaname != nullptr) {
    throw std::runtime_error("Schema name not supported.");
  }
  if (base_table.sample != nullptr) {
    throw std::runtime_error("Sample options not supported.");
  }

  std::string_view table_name = base_table.relname;

  return std::make_unique<BaseTable>(table_name,
                                     TransformAlias(base_table.alias));
}

std::unique_ptr<Table> TransformTable(duckdb_libpgquery::PGNode& base) {
  switch (base.type) {
    case duckdb_libpgquery::T_PGRangeVar:
      return TransformBaseTable(
          reinterpret_cast<duckdb_libpgquery::PGRangeVar&>(base));
    default:
      throw std::runtime_error("Unsupported from type.");
  }
}

std::unique_ptr<Table> TransformFrom(duckdb_libpgquery::PGList& from) {
  if (from.length > 1) {
    std::vector<std::unique_ptr<Table>> result;
    for (auto node = from.head; node != nullptr; node = node->next) {
      auto base =
          reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
      result.emplace_back(TransformTable(*base));
    }
    return std::make_unique<CrossProductTable>(std::move(result));
  }

  auto base =
      reinterpret_cast<duckdb_libpgquery::PGNode*>(from.head->data.ptr_value);
  return TransformTable(*base);
}

}  // namespace kush::parse