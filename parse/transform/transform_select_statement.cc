#include "parse/transform/transform_select_statement.h"

#include <memory>

#include "parse/statement/select_statement.h"
#include "parse/transform/transform_expression.h"
#include "parse/transform/transform_from.h"
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<SelectStatement> TransformSelectStatement(
    libpgquery::PGNode& node) {
  auto& stmt = reinterpret_cast<libpgquery::PGSelectStmt&>(node);

  if (stmt.withClause != nullptr) {
    throw std::runtime_error("WITH clause not supported.");
  }

  if (stmt.windowClause != nullptr) {
    throw std::runtime_error("WINDOW clause not supported.");
  }

  if (stmt.intoClause != nullptr) {
    throw std::runtime_error("INTO clause not supported.");
  }

  if (stmt.lockingClause != nullptr) {
    throw std::runtime_error("Locking clause not supported.");
  }

  if (stmt.distinctClause != nullptr) {
    throw std::runtime_error("Distinct not supported.");
  }

  if (stmt.valuesLists != nullptr) {
    throw std::runtime_error("Value lists not supported.");
  }

  if (stmt.sampleOptions != nullptr) {
    throw std::runtime_error("SAMPLE not supported.");
  }

  std::vector<std::unique_ptr<Expression>> selects;
  std::unique_ptr<Table> from;
  std::unique_ptr<Expression> where;

  if (stmt.targetList == nullptr) {
    throw std::runtime_error("Missing selects.");
  }
  selects = TransformExpressionList(*stmt.targetList);

  if (stmt.fromClause == nullptr) {
    throw std::runtime_error("Missing from.");
  }
  from = TransformFrom(*stmt.fromClause);

  if (stmt.whereClause != nullptr) {
    where = TransformExpression(*stmt.whereClause);
  }

  return std::make_unique<SelectStatement>(std::move(selects), std::move(from),
                                           std::move(where));
}

}  // namespace kush::parse