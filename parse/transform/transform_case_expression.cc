#include "parse/transform/transform_case_expression.h"

#include <memory>
#include <vector>

#include "parse/expression/arithmetic_expression.h"
#include "parse/expression/case_expression.h"
#include "parse/expression/literal_expression.h"
#include "parse/transform/transform_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<CaseExpression> TransformCaseExpression(
    duckdb_libpgquery::PGCaseExpr &expr) {
  std::vector<
      std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>
      cases;
  for (auto cell = expr.args->head; cell != nullptr; cell = cell->next) {
    auto w =
        reinterpret_cast<duckdb_libpgquery::PGCaseWhen *>(cell->data.ptr_value);
    auto test_raw = TransformExpression(
        *reinterpret_cast<duckdb_libpgquery::PGNode *>(w->expr));

    std::unique_ptr<Expression> when;
    if (expr.arg) {
      auto arg = TransformExpression(
          *reinterpret_cast<duckdb_libpgquery::PGNode *>(expr.arg));
      when = std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::EQ, std::move(arg),
          std::move(test_raw));
    } else {
      when = move(test_raw);
    }

    auto then = TransformExpression(
        *reinterpret_cast<duckdb_libpgquery::PGNode *>(w->result));
    cases.emplace_back(std::move(when), std::move(then));
  }

  std::unique_ptr<Expression> fallthrough;
  if (expr.defresult != nullptr) {
    fallthrough = TransformExpression(
        *reinterpret_cast<duckdb_libpgquery::PGNode *>(expr.defresult));
  } else {
    fallthrough = std::make_unique<LiteralExpression>(nullptr);
  }

  return std::make_unique<CaseExpression>(std::move(cases),
                                          std::move(fallthrough));
}

}  // namespace kush::parse