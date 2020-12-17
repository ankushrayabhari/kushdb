#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp_translator.h"
#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.insert("table").insert("i1", SqlType::INT, "sample/int1.kdb");
  db.insert("table1").insert("i2", SqlType::INT, "sample/int2.kdb");
  db.insert("table1").insert("bi1", SqlType::BIGINT, "sample/bigint1.kdb");
  db.insert("table2").insert("i3", SqlType::INT, "sample/int3.kdb");
  db.insert("table2").insert("i4", SqlType::INT, "sample/int4.kdb");
  db.insert("table2").insert("t1", SqlType::TEXT, "sample/text1.kdb");

  // Scan(table)
  std::unique_ptr<Operator> scan_table;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumn("i1", SqlType::INT);
    scan_table = std::make_unique<ScanOperator>(std::move(schema), "table");
  }

  // Select(table_scan, x1 < 100000000)
  std::unique_ptr<Operator> select_table;
  {
    auto x1 = std::make_unique<ColumnRefExpression>(
        0, scan_table->Schema().GetColumnIndex("i1"));
    auto x1_lt_c = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(x1),
        std::make_unique<LiteralExpression>(100000000));

    OperatorSchema schema;
    for (const auto& column : scan_table->Schema().Columns()) {
      schema.AddDerivedColumn(
          column.Name(), column.Type(),
          std::make_unique<ColumnRefExpression>(
              0, scan_table->Schema().GetColumnIndex(column.Name())));
    }

    select_table = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_table), std::move(x1_lt_c));
  }

  // scan(table1)
  std::unique_ptr<Operator> scan_table1;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumn("i2", SqlType::INT);
    schema.AddGeneratedColumn("bi1", SqlType::BIGINT);
    scan_table1 = std::make_unique<ScanOperator>(std::move(schema), "table1");
  }

  // select(table) join table1 ON x1 = x2
  std::unique_ptr<Operator> table_join_table1;
  {
    OperatorSchema schema;
    for (const auto& column : select_table->Schema().Columns()) {
      schema.AddDerivedColumn(
          column.Name(), column.Type(),
          std::make_unique<ColumnRefExpression>(
              0, select_table->Schema().GetColumnIndex(column.Name())));
    }
    for (const auto& column : scan_table1->Schema().Columns()) {
      schema.AddDerivedColumn(
          column.Name(), column.Type(),
          std::make_unique<ColumnRefExpression>(
              1, scan_table1->Schema().GetColumnIndex(column.Name())));
    }

    auto x1 = std::make_unique<ColumnRefExpression>(
        0, select_table->Schema().GetColumnIndex("i1"));
    auto x2 = std::make_unique<ColumnRefExpression>(
        1, scan_table1->Schema().GetColumnIndex("i2"));
    table_join_table1 = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(select_table), std::move(scan_table1),
        std::move(x1), std::move(x2));
  }

  // scan(table2)
  std::unique_ptr<Operator> scan_table2;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumn("i3", SqlType::INT);
    schema.AddGeneratedColumn("i4", SqlType::INT);
    schema.AddGeneratedColumn("t1", SqlType::TEXT);
    scan_table2 = std::make_unique<ScanOperator>(std::move(schema), "table2");
  }

  // (select(table) join table1 ON x1 = x2) join table2 ON x2 = x3
  std::unique_ptr<Operator> table_join_table1_join_table2;
  {
    OperatorSchema schema;
    for (const auto& column : table_join_table1->Schema().Columns()) {
      schema.AddDerivedColumn(
          column.Name(), column.Type(),
          std::make_unique<ColumnRefExpression>(
              0, table_join_table1->Schema().GetColumnIndex(column.Name())));
    }
    for (const auto& column : scan_table2->Schema().Columns()) {
      schema.AddDerivedColumn(
          column.Name(), column.Type(),
          std::make_unique<ColumnRefExpression>(
              1, scan_table2->Schema().GetColumnIndex(column.Name())));
    }

    auto x2 = std::make_unique<ColumnRefExpression>(
        0, table_join_table1->Schema().GetColumnIndex("i2"));
    auto x3 = std::make_unique<ColumnRefExpression>(
        1, scan_table2->Schema().GetColumnIndex("i3"));
    table_join_table1_join_table2 = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(table_join_table1), std::move(scan_table2),
        std::move(x2), std::move(x3));
  }

  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(
      std::move(table_join_table1_join_table2));

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
