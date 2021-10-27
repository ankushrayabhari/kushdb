#include "plan/planner.h"

#include "catalog/catalog_manager.h"
#include "parse/expression/aggregate_expression.h"
#include "parse/expression/arithmetic_expression.h"
#include "parse/expression/case_expression.h"
#include "parse/expression/column_ref_expression.h"
#include "parse/expression/comparison_expression.h"
#include "parse/expression/expression.h"
#include "parse/expression/in_expression.h"
#include "parse/expression/literal_expression.h"
#include "parse/statement/select_statement.h"
#include "parse/table/table.h"
#include "plan/operator/cross_product_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"

namespace kush::plan {

std::unique_ptr<Operator> Plan(const parse::Table& table) {
  if (auto base_table = dynamic_cast<const parse::BaseTable*>(&table)) {
    auto table_name = base_table->Name();
    auto alias = base_table->Alias();
    const auto& db = catalog::CatalogManager::Get().Current();
    if (!db.Contains(table_name)) {
      throw std::runtime_error("Unknown table");
    }

    const auto& table = db[table_name];

    // todo what to do with alias?
    OperatorSchema schema;
    schema.AddGeneratedColumns(table);
    return std::make_unique<plan::ScanOperator>(std::move(schema), table);
  }

  if (auto cross_product =
          dynamic_cast<const parse::CrossProductTable*>(&table)) {
    OperatorSchema schema;
    std::vector<std::unique_ptr<Operator>> children;

    int child_idx = 0;
    for (auto table : cross_product->Children()) {
      children.push_back(Plan(table.get()));
      schema.AddPassthroughColumns(*children.back(), child_idx);
      child_idx++;
    }

    return std::make_unique<CrossProductOperator>(std::move(schema),
                                                  std::move(children));
  }

  throw std::runtime_error("Unknown table type.");
}

std::unique_ptr<Expression> Plan(const parse::AggregateExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::UnaryArithmeticExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(
    const parse::BinaryArithmeticExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::CaseExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::ColumnRefExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::ComparisonExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::InExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::LiteralExpression& expr) {
  return nullptr;
}

std::unique_ptr<Expression> Plan(const parse::Expression& expr) {
  if (auto v = dynamic_cast<const parse::AggregateExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::UnaryArithmeticExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::BinaryArithmeticExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::CaseExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::ColumnRefExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::ComparisonExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::InExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::LiteralExpression*>(&expr)) {
    return Plan(*v);
  }

  throw std::runtime_error("Unknown expression");
}

std::unique_ptr<Operator> Plan(const parse::SelectStatement& stmt) {
  auto result = Plan(stmt.From());

  if (auto where = stmt.Where()) {
    auto expr = Plan(*where);

    OperatorSchema schema;
    schema.AddPassthroughColumns(*result);
    result = std::make_unique<SelectOperator>(
        std::move(schema), std::move(result), std::move(expr));
  }

  return result;
}

}  // namespace kush::plan