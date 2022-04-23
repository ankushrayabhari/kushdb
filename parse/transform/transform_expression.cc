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
#include "third_party/libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformResTarget(libpgquery::PGResTarget& root) {
  auto expr = TransformExpression(*root.val);

  if (root.name != nullptr) {
    expr->SetAlias(root.name);
  }

  return expr;
}

std::unique_ptr<Expression> TransformExpression(libpgquery::PGNode& expr) {
  switch (expr.type) {
    case libpgquery::T_PGResTarget:
      return TransformResTarget(
          reinterpret_cast<libpgquery::PGResTarget&>(expr));
    case libpgquery::T_PGColumnRef:
      return TransformColumnRefExpression(
          reinterpret_cast<libpgquery::PGColumnRef&>(expr));
    case libpgquery::T_PGAConst:
      return TransformLiteralExpression(
          reinterpret_cast<libpgquery::PGAConst&>(expr).val);
    case libpgquery::T_PGAExpr:
      return TransformArithmeticExpression(
          reinterpret_cast<libpgquery::PGAExpr&>(expr));
    case libpgquery::T_PGNullTest:
      return TransformNullTestExpression(
          reinterpret_cast<libpgquery::PGNullTest&>(expr));
    case libpgquery::T_PGBoolExpr:
      return TransformBoolExpression(
          reinterpret_cast<libpgquery::PGBoolExpr&>(expr));
    case libpgquery::T_PGCaseExpr:
      return TransformCaseExpression(
          reinterpret_cast<libpgquery::PGCaseExpr&>(expr));
    case libpgquery::T_PGFuncCall:
      return TransformFunctionCallExpression(
          reinterpret_cast<libpgquery::PGFuncCall&>(expr));
    default:
      throw std::runtime_error("Expr not implemented: " +
                               std::to_string(expr.type));
  }
}

std::vector<std::unique_ptr<Expression>> TransformExpressionList(
    libpgquery::PGList& list) {
  std::vector<std::unique_ptr<Expression>> result;
  for (auto node = list.head; node != nullptr; node = node->next) {
    auto target = reinterpret_cast<libpgquery::PGNode*>(node->data.ptr_value);
    result.push_back(TransformExpression(*target));
  }
  return result;
}

}  // namespace kush::parse