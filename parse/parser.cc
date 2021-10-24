#include "parse/parser.h"

#include <memory>
#include <stdexcept>
#include <string_view>

#include "duckdb_libpgquery/parser.h"

#include "parse/statement/statement.h"
#include "parse/transform/transform_statement.h"

namespace kush::parse {

std::vector<std::unique_ptr<Statement>> Parse(std::string_view q) {
  std::string query(q);

  duckdb_libpgquery::PostgresParser parser;
  parser.Parse(std::string(query));
  if (!parser.success) {
    throw std::runtime_error(parser.error_message + ": " + query);
  }

  std::vector<std::unique_ptr<Statement>> result;

  auto parse_tree = parser.parse_tree;
  for (auto entry = parse_tree->head; entry != nullptr; entry = entry->next) {
    result.push_back(TransformStatement(
        *reinterpret_cast<duckdb_libpgquery::PGNode*>(entry->data.ptr_value)));
  }

  return result;
}

}  // namespace kush::parse
