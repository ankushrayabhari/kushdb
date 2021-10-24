#include "parse/transform/transform_statement.h"

#include <memory>
#include <string>

#include "magic_enum.hpp"

#include "parse/statement/statement.h"
#include "parse/transform/transform_select_statement.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Statement> TransformStatement(duckdb_libpgquery::PGNode& stmt) {
  switch (stmt.type) {
    case duckdb_libpgquery::T_PGSelectStmt:
      return TransformSelectStatement(stmt);
    default:
      throw std::runtime_error("Unimplemented statement: " +
                               std::string(magic_enum::enum_name(stmt.type)));
  }
}

}  // namespace kush::parse