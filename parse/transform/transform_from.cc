#include "parse/transform/transform_from.h"

#include <memory>
#include <vector>

#include "parse/table/table.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::string TransformAlias(libpgquery::PGAlias* root) {
  if (root == nullptr) {
    return "";
  }

  if (root->colnames) {
    throw std::runtime_error("Column aliases not supported.");
  }

  return root->aliasname;
}

std::unique_ptr<BaseTable> TransformBaseTable(
    libpgquery::PGRangeVar& base_table) {
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

std::unique_ptr<Table> TransformTable(libpgquery::PGNode& base) {
  switch (base.type) {
    case libpgquery::T_PGRangeVar:
      return TransformBaseTable(
          reinterpret_cast<libpgquery::PGRangeVar&>(base));
    default:
      throw std::runtime_error("Unsupported from type.");
  }
}

std::unique_ptr<Table> TransformFrom(libpgquery::PGList& from) {
  if (from.length > 1) {
    std::vector<std::unique_ptr<Table>> result;
    for (auto node = from.head; node != nullptr; node = node->next) {
      auto base = reinterpret_cast<libpgquery::PGNode*>(node->data.ptr_value);
      result.emplace_back(TransformTable(*base));
    }
    return std::make_unique<CrossProductTable>(std::move(result));
  }

  auto base = reinterpret_cast<libpgquery::PGNode*>(from.head->data.ptr_value);
  return TransformTable(*base);
}

}  // namespace kush::parse