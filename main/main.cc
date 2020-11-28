#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "catalog/catalog.h"
#include "compilation/cpp_translator.h"
#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/sql_type.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.insert("table").insert("x1", "int32_t", "test.skdbcol");
  db.insert("table1").insert("x2", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x3", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x4", "int32_t", "test1.skdbcol");

  // Scan(table)
  std::unique_ptr<Operator> scan_table;
  {
    OperatorSchema schema;
    schema.AddColumn("x1", SqlType::INT);
    scan_table = std::make_unique<Scan>(std::move(schema), "table");
  }

  // Select(table_scan, x1 < 100000000)
  std::unique_ptr<Operator> select_table;
  {
    auto x1 = std::make_unique<ColumnRefExpression>(
        scan_table->Schema().GetColumnIndex("x1"));
    auto x1_lt_c = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(x1),
        std::make_unique<LiteralExpression<int32_t>>(100000000));
    OperatorSchema schema = scan_table->Schema();
    select_table = std::make_unique<Select>(
        std::move(schema), std::move(scan_table), std::move(x1_lt_c));
  }

  // scan(table1)
  std::unique_ptr<Operator> scan_table1;
  {
    OperatorSchema schema;
    schema.AddColumn("x2", SqlType::INT);
    scan_table1 = std::make_unique<Scan>(std::move(schema), "table1");
  }

  // select(table) join table1 ON x1 = x2
  std::unique_ptr<Operator> table_join_table1;
  {
    OperatorSchema schema;
    for (const auto& col : select_table->Schema().Columns()) {
      schema.AddColumn(col.Name(), col.Type());
    }
    for (const auto& col : scan_table1->Schema().Columns()) {
      schema.AddColumn(col.Name(), col.Type());
    }

    auto x1 = std::make_unique<ColumnRefExpression>(
        0, select_table->Schema().GetColumnIndex("x1"));
    auto x2 = std::make_unique<ColumnRefExpression>(
        1, scan_table1->Schema().GetColumnIndex("x2"));
    table_join_table1 = std::make_unique<HashJoin>(
        std::move(schema), std::move(select_table), std::move(scan_table1),
        std::move(x1), std::move(x2));
  }

  // scan(table2)
  std::unique_ptr<Operator> scan_table2;
  {
    OperatorSchema schema;
    schema.AddColumn("x3", SqlType::INT);
    schema.AddColumn("x4", SqlType::INT);
    scan_table2 = std::make_unique<Scan>(std::move(schema), "table2");
  }

  // (select(table) join table1 ON x1 = x2) join table2 ON x2 = x3
  std::unique_ptr<Operator> table_join_table1_join_table2;
  {
    OperatorSchema schema;
    for (const auto& col : table_join_table1->Schema().Columns()) {
      schema.AddColumn(col.Name(), col.Type());
    }
    for (const auto& col : scan_table2->Schema().Columns()) {
      schema.AddColumn(col.Name(), col.Type());
    }

    auto x2 = std::make_unique<ColumnRefExpression>(
        0, table_join_table1->Schema().GetColumnIndex("x2"));
    auto x3 = std::make_unique<ColumnRefExpression>(
        1, scan_table2->Schema().GetColumnIndex("x3"));
    table_join_table1_join_table2 = std::make_unique<HashJoin>(
        std::move(schema), std::move(table_join_table1), std::move(scan_table2),
        std::move(x2), std::move(x3));
  }

  std::cout << std::setw(2) << table_join_table1_join_table2->ToJson()
            << std::endl;

  return 0;
}
