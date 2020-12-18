#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp_translator.h"
#include "nlohmann/json.hpp"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.Insert("table").Insert("i1", SqlType::INT, "sample/int1.kdb");
  db.Insert("table1").Insert("i2", SqlType::INT, "sample/int2.kdb");
  db.Insert("table1").Insert("bi1", SqlType::BIGINT, "sample/bigint1.kdb");
  db.Insert("table2").Insert("i3", SqlType::INT, "sample/int3.kdb");
  db.Insert("table2").Insert("i4", SqlType::INT, "sample/int4.kdb");
  db.Insert("table2").Insert("t1", SqlType::TEXT, "sample/text1.kdb");

  // scan(table1)
  std::unique_ptr<Operator> scan_table1;
  {
    OperatorSchema schema;
    for (const auto& col : db["table1"].Columns()) {
      schema.AddGeneratedColumn(col.get().Name(), col.get().Type());
    }
    scan_table1 = std::make_unique<ScanOperator>(std::move(schema), "table1");
  }

  // Group By true -> AVG(i2)
  std::unique_ptr<Operator> group_by_agg_table1;
  {
    std::unique_ptr<Expression> expr =
        std::make_unique<LiteralExpression>(true);
    auto avg_i2 = std::make_unique<AggregateExpression>(
        AggregateType::AVG,
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0, scan_table1->Schema().GetColumnIndex("i2")));
    auto count_bi1 = std::make_unique<AggregateExpression>(
        AggregateType::COUNT,
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0, scan_table1->Schema().GetColumnIndex("bi1")));
    auto sum_i2 = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0, scan_table1->Schema().GetColumnIndex("i2")));

    OperatorSchema schema;
    schema.AddDerivedColumn(
        "avg_i2",
        std::make_unique<VirtualColumnRefExpression>(avg_i2->Type(), 0));
    schema.AddDerivedColumn(
        "count_bi1",
        std::make_unique<VirtualColumnRefExpression>(count_bi1->Type(), 1));
    schema.AddDerivedColumn(
        "sum_i2",
        std::make_unique<VirtualColumnRefExpression>(sum_i2->Type(), 2));

    group_by_agg_table1 = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(scan_table1),
        util::MakeVector(std::move(expr)),
        util::MakeVector(std::move(avg_i2), std::move(count_bi1),
                         std::move(sum_i2)));
  }

  std::unique_ptr<Operator> query =
      std::make_unique<OutputOperator>(std::move(group_by_agg_table1));

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
