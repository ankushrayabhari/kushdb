#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp_translator.h"
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

  // Scan(customer)
  std::unique_ptr<Operator> scan_customer;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["customer"], {"c_mktsegment", "c_custkey"});
    scan_customer =
        std::make_unique<ScanOperator>(std::move(schema), "customer");
  }

  // Select(c_mktsegment = 'FURNITURE')
  std::unique_ptr<Operator> select_customer;
  {
    auto c_mktsegment = std::make_unique<ColumnRefExpression>(
        SqlType::TEXT, 0,
        scan_customer->Schema().GetColumnIndex("c_mktsegment"));
    auto literal = std::make_unique<LiteralExpression>("FURNITURE");
    auto eq = std::make_unique<ComparisonExpression>(
        ComparisonType::EQ, std::move(c_mktsegment), std::move(literal));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_customer, {"c_custkey"});
    select_customer = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_customer), std::move(eq));
  }

  // Scan(orders)
  std::unique_ptr<Operator> scan_orders;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["orders"], {"o_orderdate", "o_shippriority",
                                              "o_custkey", "o_orderkey"});
    scan_orders = std::make_unique<ScanOperator>(std::move(schema), "orders");
  }

  // Select(o_orderdate < '1995-03-29')
  std::unique_ptr<Operator> select_orders;
  {
    auto o_orderdate = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_orders->Schema().GetColumnIndex("o_orderdate"));
    auto literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1995, 3, 29));
    auto lt = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(o_orderdate), std::move(literal));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_orders, {"o_orderdate", "o_shippriority",
                                                "o_custkey", "o_orderkey"});
    select_orders = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_orders), std::move(lt));
  }

  // select_customer JOIN select_orders ON c_custkey = o_custkey
  std::unique_ptr<Operator> customer_orders;
  {
    auto c_custkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 0, select_customer->Schema().GetColumnIndex("c_custkey"));
    auto o_custkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 1, select_orders->Schema().GetColumnIndex("o_custkey"));

    OperatorSchema schema;
    schema.AddPassthroughColumns(
        *select_orders, {"o_orderdate", "o_shippriority", "o_orderkey"}, 1);
    customer_orders = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(select_customer), std::move(select_orders),
        std::move(c_custkey), std::move(o_custkey));
  }

  // Scan(lineitem)
  std::unique_ptr<Operator> scan_lineitem;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_extendedprice",
                                                "l_shipdate", "l_discount"});
    scan_lineitem =
        std::make_unique<ScanOperator>(std::move(schema), "lineitem");
  }

  // Select(l_shipdate > '1995-03-29')
  std::unique_ptr<Operator> select_lineitem;
  {
    auto l_shipdate = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_lineitem->Schema().GetColumnIndex("l_shipdate"));
    auto literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1995, 3, 29));
    auto gt = std::make_unique<ComparisonExpression>(
        ComparisonType::GT, std::move(l_shipdate), std::move(literal));

    OperatorSchema schema;
    schema.AddPassthroughColumns(
        *scan_lineitem, {"l_orderkey", "l_extendedprice", "l_discount"});
    select_lineitem = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_lineitem), std::move(gt));
  }

  // customer_orders JOIN select_lineitem ON l_orderkey = o_orderkey
  std::unique_ptr<Operator> customer_orders_lineitem;
  {
    auto o_orderkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 0,
        customer_orders->Schema().GetColumnIndex("o_orderkey"));
    auto l_orderkey = std::make_unique<ColumnRefExpression>(
        SqlType::INT, 1,
        select_lineitem->Schema().GetColumnIndex("l_orderkey"));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*customer_orders,
                                 {"o_orderdate", "o_shippriority"}, 0);
    schema.AddPassthroughColumns(
        *select_lineitem, {"l_orderkey", "l_extendedprice", "l_discount"}, 1);
    customer_orders_lineitem = std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(customer_orders),
        std::move(select_lineitem), std::move(o_orderkey),
        std::move(l_orderkey));
  }

  // Group By l_orderkey, o_orderdate, o_shippriority
  std::unique_ptr<Operator> agg;
  {
    // Group by
    std::unique_ptr<Expression> l_orderkey =
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0,
            customer_orders_lineitem->Schema().GetColumnIndex("l_orderkey"));
    std::unique_ptr<Expression> o_orderdate =
        std::make_unique<ColumnRefExpression>(
            SqlType::DATE, 0,
            customer_orders_lineitem->Schema().GetColumnIndex("o_orderdate"));
    std::unique_ptr<Expression> o_shippriority =
        std::make_unique<ColumnRefExpression>(
            SqlType::INT, 0,
            customer_orders_lineitem->Schema().GetColumnIndex(
                "o_shippriority"));

    // aggregate
    auto revenue = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ArithmeticExpression>(
            ArithmeticOperatorType::MUL,
            std::make_unique<ColumnRefExpression>(
                SqlType::REAL, 0,
                customer_orders_lineitem->Schema().GetColumnIndex(
                    "l_extendedprice")),
            std::make_unique<ArithmeticExpression>(
                ArithmeticOperatorType::SUB,
                std::make_unique<LiteralExpression>(1),
                std::make_unique<ColumnRefExpression>(
                    SqlType::REAL, 0,
                    customer_orders_lineitem->Schema().GetColumnIndex(
                        "l_discount")))));

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn(
        "l_orderkey",
        std::make_unique<VirtualColumnRefExpression>(SqlType::INT, 0));
    schema.AddDerivedColumn(
        "revenue",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 3));
    schema.AddDerivedColumn(
        "o_orderdate",
        std::make_unique<VirtualColumnRefExpression>(SqlType::DATE, 1));
    schema.AddDerivedColumn(
        "o_shippriority",
        std::make_unique<VirtualColumnRefExpression>(SqlType::INT, 2));

    agg = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(customer_orders_lineitem),
        util::MakeVector(std::move(l_orderkey), std::move(o_orderdate),
                         std::move(o_shippriority)),
        util::MakeVector(std::move(revenue)));
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
