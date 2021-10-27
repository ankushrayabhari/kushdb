#include "parse/transform/transform_arithmetic_expression.h"

#include <memory>
#include <string>
#include <vector>

#include "parse/expression/arithmetic_expression.h"
#include "parse/expression/comparison_expression.h"
#include "parse/expression/expression.h"
#include "parse/expression/in_expression.h"
#include "parse/transform/transform_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformUnaryOperator(
    const std::string& op, std::unique_ptr<Expression> child) {
  throw std::runtime_error("Unknown operator: " + op);
}

std::unique_ptr<Expression> TransformBinaryOperator(
    const std::string& op, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right) {
  if (op == "=" || op == "==") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::EQ, std::move(left), std::move(right));
  }

  if (op == "!=" || op == "<>") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::NEQ, std::move(left), std::move(right));
  }

  if (op == "<") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(left), std::move(right));
  }

  if (op == ">") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::GT, std::move(left), std::move(right));
  }

  if (op == "<=") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::LEQ, std::move(left), std::move(right));
  }

  if (op == ">=") {
    return std::make_unique<ComparisonExpression>(
        ComparisonType::GEQ, std::move(left), std::move(right));
  }

  throw std::runtime_error("Unknown operator: " + op);
}

std::unique_ptr<Expression> TransformArithmeticExpression(
    duckdb_libpgquery::PGAExpr& expr) {
  auto name = std::string((reinterpret_cast<duckdb_libpgquery::PGValue*>(
                               expr.name->head->data.ptr_value))
                              ->val.str);

  switch (expr.kind) {
    case duckdb_libpgquery::PG_AEXPR_OP_ALL:
    case duckdb_libpgquery::PG_AEXPR_OP_ANY: {
      throw std::runtime_error("ANY/ALL unsupported.");
    }

    case duckdb_libpgquery::PG_AEXPR_NULLIF: {
      throw std::runtime_error("NULLIF unsupported.");
    }

    case duckdb_libpgquery::PG_AEXPR_SIMILAR: {
      throw std::runtime_error("SIMILAR unsupported.");
    }

    case duckdb_libpgquery::PG_AEXPR_DISTINCT:
    case duckdb_libpgquery::PG_AEXPR_NOT_DISTINCT: {
      throw std::runtime_error("DISTINCT unsupported.");
    }

    case duckdb_libpgquery::PG_AEXPR_IN: {
      auto child = TransformExpression(*expr.lexpr);
      auto valid =
          TransformExpressionList(*((duckdb_libpgquery::PGList*)expr.rexpr));

      auto base =
          std::make_unique<InExpression>(std::move(child), std::move(valid));

      if (name == "<>") {
        return std::make_unique<UnaryArithmeticExpression>(
            UnaryArithmeticExpressionType::NOT, std::move(base));
      } else {
        return std::move(base);
      }
    }

    // rewrite (NOT) X BETWEEN A AND B into (NOT) AND(GREATERTHANOREQUALTO(X,
    // A), LESSTHANOREQUALTO(X, B))
    case duckdb_libpgquery::PG_AEXPR_BETWEEN:
    case duckdb_libpgquery::PG_AEXPR_NOT_BETWEEN: {
      auto between_args =
          reinterpret_cast<duckdb_libpgquery::PGList*>(expr.rexpr);
      if (between_args->length != 2 || !between_args->head->data.ptr_value ||
          !between_args->tail->data.ptr_value) {
        throw std::runtime_error("(NOT) BETWEEN needs two args");
      }

      auto left_input = TransformExpression(*expr.lexpr);
      auto right_input = TransformExpression(*expr.lexpr);
      auto between_left =
          TransformExpression(*reinterpret_cast<duckdb_libpgquery::PGNode*>(
              between_args->head->data.ptr_value));
      auto between_right =
          TransformExpression(*reinterpret_cast<duckdb_libpgquery::PGNode*>(
              between_args->tail->data.ptr_value));

      auto geq = std::make_unique<ComparisonExpression>(
          ComparisonType::GEQ, std::move(left_input), move(between_left));

      auto leq = std::make_unique<ComparisonExpression>(
          ComparisonType::LEQ, std::move(right_input), move(between_left));

      auto both = std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::AND, std::move(leq), std::move(geq));

      if (expr.kind == duckdb_libpgquery::PG_AEXPR_BETWEEN) {
        return std::move(both);
      } else {
        return std::make_unique<UnaryArithmeticExpression>(
            UnaryArithmeticExpressionType::NOT, std::move(both));
      }
    }

    default:
      break;
  }

  if (expr.lexpr == nullptr && expr.rexpr != nullptr) {
    return TransformUnaryOperator(name, TransformExpression(*expr.rexpr));
  }

  if (expr.lexpr != nullptr && expr.rexpr != nullptr) {
    return TransformBinaryOperator(name, TransformExpression(*expr.lexpr),
                                   TransformExpression(*expr.rexpr));
  }

  throw std::runtime_error("Invalid expression.");
}

std::unique_ptr<Expression> TransformNullTest(
    duckdb_libpgquery::PGNullTest& expr) {
  auto arg = TransformExpression(
      *reinterpret_cast<duckdb_libpgquery::PGNode*>(expr.arg));
  if (expr.argisrow) {
    throw std::runtime_error("IS NULL argisrow not supported.");
  }

  auto base = std::make_unique<UnaryArithmeticExpression>(
      UnaryArithmeticExpressionType::IS_NULL, std::move(arg));
  if (expr.nulltesttype == duckdb_libpgquery::PG_IS_NULL) {
    return std::move(base);
  } else {
    return std::make_unique<UnaryArithmeticExpression>(
        UnaryArithmeticExpressionType::NOT, std::move(base));
  }
}

}  // namespace kush::parse