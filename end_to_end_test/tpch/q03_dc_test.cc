#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/parameters.h"
#include "end_to_end_test/schema.h"
#include "end_to_end_test/test_macros.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "plan/operator/cross_product_operator.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = EnumSchema();

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_mktsegment", "c_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Select(c_mktsegment = 'AUTOMOBILE')
std::unique_ptr<Operator> SelectCustomer() {
  auto scan_customer = ScanCustomer();

  auto eq = Eq(ColRef(scan_customer, "c_mktsegment"), Literal("AUTOMOBILE"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_customer, {"c_custkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_customer), std::move(eq));
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderdate", "o_shippriority",
                                            "o_custkey", "o_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate < '1995-03-30')
std::unique_ptr<Operator> SelectOrders() {
  auto scan_orders = ScanOrders();
  auto lt = Lt(ColRef(scan_orders, "o_orderdate"), Literal(1995, 3, 30));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_orders, {"o_orderdate", "o_shippriority",
                                              "o_custkey", "o_orderkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_orders), std::move(lt));
}

// select_customer JOIN select_orders ON c_custkey = o_custkey
std::unique_ptr<Operator> CustomerOrders() {
  auto customer = SelectCustomer();
  auto orders = SelectOrders();

  auto c_custkey = ColRef(customer, "c_custkey", 0);
  auto o_custkey = ColRef(orders, "o_custkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *orders, {"o_orderdate", "o_shippriority", "o_orderkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(customer), std::move(orders),
      util::MakeVector(std::move(c_custkey)),
      util::MakeVector(std::move(o_custkey)));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_extendedprice",
                                              "l_shipdate", "l_discount"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate > '1995-03-30')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();
  auto gt = Gt(ColRef(lineitem, "l_shipdate"), Literal(1995, 3, 30));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem,
                               {"l_orderkey", "l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(gt));
}

// customer_orders JOIN select_lineitem ON l_orderkey = o_orderkey
std::unique_ptr<Operator> CustomerOrdersLineitem() {
  auto customer_orders = CustomerOrders();
  auto lineitem = SelectLineitem();

  auto o_orderkey = ColRef(customer_orders, "o_orderkey", 0);
  auto l_orderkey = ColRef(lineitem, "l_orderkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*customer_orders,
                               {"o_orderdate", "o_shippriority"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_orderkey", "l_extendedprice", "l_discount"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(customer_orders), std::move(lineitem),
      util::MakeVector(std::move(o_orderkey)),
      util::MakeVector(std::move(l_orderkey)));
}

// Group By l_orderkey, o_orderdate, o_shippriority -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = CustomerOrdersLineitem();

  // Group by
  auto l_orderkey = ColRefE(base, "l_orderkey");
  auto o_orderdate = ColRefE(base, "o_orderdate");
  auto o_shippriority = ColRefE(base, "o_shippriority");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("l_orderkey", VirtColRef(l_orderkey, 0));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 3));
  schema.AddDerivedColumn("o_orderdate", VirtColRef(o_orderdate, 1));
  schema.AddDerivedColumn("o_shippriority", VirtColRef(o_shippriority, 2));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(l_orderkey), std::move(o_orderdate),
                       std::move(o_shippriority)),
      util::MakeVector(std::move(revenue)));
}

// Order By revenue desc, o_orderdate
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto revenue = ColRef(agg, "revenue");
  auto o_orderdate = ColRef(agg, "o_orderdate");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(revenue), std::move(o_orderdate)),
      std::vector<bool>{false, true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q03) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q03_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

NORMAL_TEST(TPCHTest)
