#include "parse/transform/transform_expression.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "magic_enum.hpp"

#include "parse/expression/expression.h"
#include "parse/transform/transform_arithmetic_expression.h"
#include "parse/transform/transform_column_ref_expression.h"
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
    /*
    case duckdb_libpgquery::T_PGFuncCall:
      return TransformFuncCall(
          reinterpret_cast<duckdb_libpgquery::PGFuncCall*>(node));
    case duckdb_libpgquery::T_PGBoolExpr:
      return TransformBoolExpr(
          reinterpret_cast<duckdb_libpgquery::PGBoolExpr*>(node));
    case duckdb_libpgquery::T_PGTypeCast:
      return TransformTypeCast(
          reinterpret_cast<duckdb_libpgquery::PGTypeCast*>(node));
    case duckdb_libpgquery::T_PGCaseExpr:
      return TransformCase(
          reinterpret_cast<duckdb_libpgquery::PGCaseExpr*>(node));
    case duckdb_libpgquery::T_PGSubLink:
      return TransformSubquery(
          reinterpret_cast<duckdb_libpgquery::PGSubLink*>(node));
    case duckdb_libpgquery::T_PGCoalesceExpr:
      return TransformCoalesce(
          reinterpret_cast<duckdb_libpgquery::PGAExpr*>(node));
    case duckdb_libpgquery::T_PGNullTest:
      return TransformNullTest(
          reinterpret_cast<duckdb_libpgquery::PGNullTest*>(node));
    case duckdb_libpgquery::T_PGParamRef:
      return TransformParamRef(
          reinterpret_cast<duckdb_libpgquery::PGParamRef*>(node));
    case duckdb_libpgquery::T_PGNamedArgExpr:
      return TransformNamedArg(
          reinterpret_cast<duckdb_libpgquery::PGNamedArgExpr*>(node));
    case duckdb_libpgquery::T_PGSQLValueFunction:
      return TransformSQLValueFunction(
          reinterpret_cast<duckdb_libpgquery::PGSQLValueFunction*>(node));
    case duckdb_libpgquery::T_PGSetToDefault:
      return make_unique<DefaultExpression>();
    case duckdb_libpgquery::T_PGCollateClause:
      return TransformCollateExpr(
          reinterpret_cast<duckdb_libpgquery::PGCollateClause*>(node));
    case duckdb_libpgquery::T_PGIntervalConstant:
      return TransformInterval(
          reinterpret_cast<duckdb_libpgquery::PGIntervalConstant*>(node));
    case duckdb_libpgquery::T_PGLambdaFunction:
      return TransformLambda(
          reinterpret_cast<duckdb_libpgquery::PGLambdaFunction*>(node));
    case duckdb_libpgquery::T_PGAIndirection:
      return TransformArrayAccess(
          reinterpret_cast<duckdb_libpgquery::PGAIndirection*>(node));
    case duckdb_libpgquery::T_PGPositionalReference:
      return TransformPositionalReference(
          reinterpret_cast<duckdb_libpgquery::PGPositionalReference*>(node));
    case duckdb_libpgquery::T_PGGroupingFunc:
      return TransformGroupingFunction(
          reinterpret_cast<duckdb_libpgquery::PGGroupingFunc*>(node));
    */
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