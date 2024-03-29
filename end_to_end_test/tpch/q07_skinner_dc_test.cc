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
#include "plan/operator/scan_select_operator.h"
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

// Select(n_name in 'EGYPT' or n_name in 'KENYA')
std::unique_ptr<Operator> SelectNation() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});

  auto cond = Exp(Or(util::MakeVector(
      Exp(Eq(VirtColRef(scan_schema, "n_name"), Literal("EGYPT"sv))),
      Exp(Eq(VirtColRef(scan_schema, "n_name"), Literal("KENYA"sv))))));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"n_nationkey", "n_name"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["nation"],
      util::MakeVector(std::move(cond)));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_custkey", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderkey", "o_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(l_shipdate >= '1995-01-01' and l_shipdate <= '1996-12-31')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"], {"l_shipdate", "l_extendedprice", "l_discount",
                       "l_suppkey", "l_orderkey"});

  auto ge =
      Exp(Geq(VirtColRef(scan_schema, "l_shipdate"), Literal(1995, 1, 1)));
  auto le =
      Exp(Leq(VirtColRef(scan_schema, "l_shipdate"), Literal(1996, 12, 31)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"l_shipdate", "l_extendedprice", "l_discount", "l_suppkey",
                    "l_orderkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(ge), std::move(le)));
}

// supplier
// JOIN lineitem ON s_suppkey = l_suppkey
// JOIN orders ON o_orderkey = l_orderkey
// JOIN customer ON c_custkey = o_custkey
// JOIN nation n1 ON s_nationkey = n1.n_nationkey
// JOIN nation n2 ON c_nationkey = n2.n_nationkey AND (
//   (n1.n_name = 'EGYPT' and n2.n_name = 'KENYA')
//   or (n1.n_name = 'KENYA' and n2.n_name = 'EGYPT')
// )
std::unique_ptr<Operator> Join() {
  auto supplier = ScanSupplier();
  auto lineitem = SelectLineitem();
  auto orders = ScanOrders();
  auto customer = ScanCustomer();
  auto n1 = SelectNation();
  auto n2 = SelectNation();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 0), ColRef(lineitem, "l_suppkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 1), ColRef(orders, "o_orderkey", 2)));
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 2), ColRef(customer, "c_custkey", 3)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 0), ColRef(n1, "n_nationkey", 4)));
  conditions.push_back(
      Eq(ColRef(customer, "c_nationkey", 3), ColRef(n2, "n_nationkey", 5)));
  {
    std::unique_ptr<Expression> eq1 =
        Eq(ColRef(n1, "n_name", 4), Literal("EGYPT"sv));
    std::unique_ptr<Expression> eq2 =
        Eq(ColRef(n2, "n_name", 5), Literal("KENYA"sv));
    std::unique_ptr<Expression> eq3 =
        Eq(ColRef(n1, "n_name", 4), Literal("KENYA"sv));
    std::unique_ptr<Expression> eq4 =
        Eq(ColRef(n2, "n_name", 5), Literal("EGYPT"sv));

    std::unique_ptr<Expression> or1 =
        And(util::MakeVector(std::move(eq1), std::move(eq2)));
    std::unique_ptr<Expression> or2 =
        And(util::MakeVector(std::move(eq3), std::move(eq4)));
    conditions.push_back(Or(util::MakeVector(std::move(or1), std::move(or2))));
  }

  auto l_year = Extract(ColRef(lineitem, "l_shipdate", 1), ExtractValue::YEAR);
  auto volume = Mul(ColRef(lineitem, "l_extendedprice", 1),
                    Sub(Literal(1.0), ColRef(lineitem, "l_discount", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumn(*n1, "n_name", "supp_nation", 4);
  schema.AddPassthroughColumn(*n2, "n_name", "cust_nation", 5);
  schema.AddDerivedColumn("l_year", std::move(l_year));
  schema.AddDerivedColumn("volume", std::move(volume));
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(supplier), std::move(lineitem),
                       std::move(orders), std::move(customer), std::move(n1),
                       std::move(n2)),
      std::move(conditions));
}

// Group By supp_nation, cust_nation, l_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto supp_nation = ColRefE(base, "supp_nation");
  auto cust_nation = ColRefE(base, "cust_nation");
  auto l_year = ColRefE(base, "l_year");

  // aggregate
  auto revenue = Sum(ColRef(base, "volume"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("supp_nation", VirtColRef(supp_nation, 0));
  schema.AddDerivedColumn("cust_nation", VirtColRef(cust_nation, 1));
  schema.AddDerivedColumn("l_year", VirtColRef(l_year, 2));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 3));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(supp_nation), std::move(cust_nation),
                       std::move(l_year)),
      util::MakeVector(std::move(revenue)));
}

// Order By supp_nation, cust_nation, l_year
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto supp_nation = ColRef(agg, "supp_nation");
  auto cust_nation = ColRef(agg, "cust_nation");
  auto l_year = ColRef(agg, "l_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(supp_nation), std::move(cust_nation),
                       std::move(l_year)),
      std::vector<bool>{true, true, true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q07Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q07_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

SKINNER_TEST(TPCHTest)