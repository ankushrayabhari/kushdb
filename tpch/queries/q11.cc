#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
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
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db = Schema();

  // Scan(nation)
  std::unique_ptr<Operator> scan_nation;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["nation"], {"n_name", "n_nationkey"});
    scan_nation = std::make_unique<ScanOperator>(std::move(schema), "nation");
  }

  // Select(n_name = 'ARGENTINA')
  std::unique_ptr<Operator> select_nation;
  {
    auto n_name = std::make_unique<ColumnRefExpression>(
        SqlType::TEXT, 0, scan_nation->Schema().GetColumnIndex("n_name"));
    auto literal = std::make_unique<LiteralExpression>("ARGENTINA");
    auto eq = std::make_unique<ComparisonExpression>(
        ComparisonType::EQ, std::move(n_name), std::move(literal));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_nation, {"n_nationkey"});
    select_nation = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_nation), std::move(eq));
  }

  // Scan(supplier)
  std::unique_ptr<Operator> scan_supplier;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
    scan_supplier =
        std::make_unique<ScanOperator>(std::move(schema), "supplier");
  }

  // select_nation JOIN supplier ON s_nationkey = n_nationkey
  std::unique_ptr<Operator> nation_supplier;
  {
    auto n_nationkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 0, select_nation->Schema().GetColumnIndex("n_nationkey"));
    auto s_nationkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 1, scan_supplier->Schema().GetColumnIndex("s_nationkey"));

    std::vector<std::string> columns;
    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_supplier, {"s_suppkey"}, 1);
    nation_supplier = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(select_nation), std::move(scan_supplier),
        std::move(n_nationkey), std::move(s_nationkey));
  }

  // Scan(partsupp)
  std::unique_ptr<Operator> scan_partsupp;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["partsupp"], {"ps_partkey", "ps_supplycost",
                                                "ps_availqty", "ps_suppkey"});
    scan_partsupp =
        std::make_unique<ScanOperator>(std::move(schema), "partsupp");
  }

  // nation_supplier JOIN parsupp ON ps_suppkey = s_suppkey
  std::unique_ptr<Operator> nation_supplier_partsupp;
  {
    auto s_suppkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 0, nation_supplier->Schema().GetColumnIndex("s_suppkey"));
    auto ps_suppkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 1, scan_partsupp->Schema().GetColumnIndex("ps_suppkey"));

    std::vector<std::string> columns;
    OperatorSchema schema;
    schema.AddPassthroughColumns(
        *scan_partsupp, {"ps_partkey", "ps_supplycost", "ps_availqty"}, 1);
    nation_supplier_partsupp = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(nation_supplier), std::move(scan_partsupp),
        std::move(s_suppkey), std::move(ps_suppkey));
  }

  // Group By ps_partkey -> AGG
  std::unique_ptr<Operator> agg;
  {
    // Group by
    std::unique_ptr<Expression> ps_partkey =
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0,
            nation_supplier_partsupp->Schema().GetColumnIndex("ps_partkey"));

    // aggregate
    auto value = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ArithmeticExpression>(
            ArithmeticOperatorType::MUL,
            std::make_unique<ColumnRefExpression>(
                SqlType::REAL, 0,
                nation_supplier_partsupp->Schema().GetColumnIndex(
                    "ps_supplycost")),
            std::make_unique<ColumnRefExpression>(
                SqlType::INT, 0,
                nation_supplier_partsupp->Schema().GetColumnIndex(
                    "ps_availqty"))));

    OperatorSchema schema;
    schema.AddDerivedColumn(
        "ps_partkey",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 0));
    schema.AddDerivedColumn(
        "value",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 1));

    agg = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(nation_supplier_partsupp),
        util::MakeVector(std::move(ps_partkey)),
        util::MakeVector(std::move(value)));
  }

  std::unique_ptr<Operator> query =
      std::make_unique<OutputOperator>(std::move(agg));

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
