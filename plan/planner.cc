#include "plan/planner.h"

#include "catalog/catalog_manager.h"
#include "parse/statement/select_statement.h"
#include "parse/table/table.h"
#include "plan/operator/cross_product_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/scan_operator.h"

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

std::unique_ptr<Operator> Plan(const parse::SelectStatement& stmt) {
  auto base = Plan(stmt.From());
  return base;
}

}  // namespace kush::plan