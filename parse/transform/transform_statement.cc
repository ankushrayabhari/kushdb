#include "parse/transform/transform_statement.h"

#include <memory>
#include <string>

#include "magic_enum.hpp"

#include "parse/statement/statement.h"
#include "parse/transform/transform_select_statement.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Statement> TransformStatement(libpgquery::PGNode& stmt) {
  switch (stmt.type) {
    case libpgquery::T_PGRawStmt: {
      auto& raw_stmt = (libpgquery::PGRawStmt&)stmt;
      return TransformStatement(*raw_stmt.stmt);
    }
    case libpgquery::T_PGSelectStmt:
      return TransformSelectStatement(stmt);
    default:
      throw std::runtime_error("Unimplemented statement: " +
                               std::to_string(stmt.type));
  }
}

}  // namespace kush::parse