#include "parse/transform/transform_expression.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "magic_enum.hpp"

#include "parse/expression/expression.h"
#include "parse/transform/transform_arithmetic_expression.h"
#include "parse/transform/transform_case_expression.h"
#include "parse/transform/transform_column_ref_expression.h"
#include "parse/transform/transform_function_call_expression.h"
#include "parse/transform/transform_literal_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformResTarget(
    duckdb_libpgquery::PGResTarget& root) {
  auto expr = TransformExpression(*root.val);

  if (root.name != nullptr) {
    expr->SetAlias(root.name);
  }

  return expr;
}

std::unique_ptr<Expression> TransformExpression(
    duckdb_libpgquery::PGNode& expr) {
  switch (expr.type) {
    case duckdb_libpgquery::T_PGResTarget:
      return TransformResTarget(
          reinterpret_cast<duckdb_libpgquery::PGResTarget&>(expr));
    case duckdb_libpgquery::T_PGColumnRef:
      return TransformColumnRefExpression(
          reinterpret_cast<duckdb_libpgquery::PGColumnRef&>(expr));
    case duckdb_libpgquery::T_PGAConst:
      return TransformLiteralExpression(
          reinterpret_cast<duckdb_libpgquery::PGAConst&>(expr).val);
    case duckdb_libpgquery::T_PGAExpr:
      return TransformArithmeticExpression(
          reinterpret_cast<duckdb_libpgquery::PGAExpr&>(expr));
    case duckdb_libpgquery::T_PGNullTest:
      return TransformNullTestExpression(
          reinterpret_cast<duckdb_libpgquery::PGNullTest&>(expr));
    case duckdb_libpgquery::T_PGBoolExpr:
      return TransformBoolExpression(
          reinterpret_cast<duckdb_libpgquery::PGBoolExpr&>(expr));
    case duckdb_libpgquery::T_PGCaseExpr:
      return TransformCaseExpression(
          reinterpret_cast<duckdb_libpgquery::PGCaseExpr&>(expr));
    case duckdb_libpgquery::T_PGFuncCall:
      return TransformFunctionCallExpression(
          reinterpret_cast<duckdb_libpgquery::PGFuncCall&>(expr));
    default:
      throw std::runtime_error("Expr not implemented: " +
                               std::to_string(expr.type));
  }
}

std::vector<std::unique_ptr<Expression>> TransformExpressionList(
    duckdb_libpgquery::PGList& list) {
  std::vector<std::unique_ptr<Expression>> result;
  for (auto node = list.head; node != nullptr; node = node->next) {
    auto target =
        reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
    result.push_back(TransformExpression(*target));
  }
  return result;
}

}  // namespace kush::parse