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
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/cross_product_operator.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "plan/operator/simd_scan_select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = EnumSchema();

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["region"], {"r_name", "r_regionkey"});

  std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>> filters(
      2);
  filters[0].emplace_back(
      Eq(VirtColRef(scan_schema, "r_name"), Literal("MIDDLE EAST"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"r_regionkey"});
  return std::make_unique<SimdScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["region"],
      std::move(filters));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_nationkey", "n_regionkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> RegionNation() {
  auto region = SelectRegion();
  auto nation = ScanNation();

  auto r_regionkey = ColRef(region, "r_regionkey", 0);
  auto n_regionkey = ColRef(nation, "n_regionkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_nationkey", "n_name"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      util::MakeVector(std::move(r_regionkey)),
      util::MakeVector(std::move(n_regionkey)));
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_custkey", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// region_nation JOIN customer ON n_nationkey = c_nationkey
std::unique_ptr<Operator> RegionNationCustomer() {
  auto region_nation = RegionNation();
  auto customer = ScanCustomer();

  auto n_nationkey = ColRef(region_nation, "n_nationkey", 0);
  auto c_nationkey = ColRef(customer, "c_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region_nation, {"n_nationkey", "n_name"}, 0);
  schema.AddPassthroughColumns(*customer, {"c_custkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation), std::move(customer),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(c_nationkey)));
}

// Select(o_orderdate >= date '1993-01-01' and o_orderdate < date '1994-01-01')
std::unique_ptr<Operator> SelectOrders() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["orders"],
                                  {"o_custkey", "o_orderkey", "o_orderdate"});

  std::vector<std::vector<std::unique_ptr<BinaryArithmeticExpression>>> filters(
      3);
  filters[2].push_back(
      Geq(VirtColRef(scan_schema, "o_orderdate"), Literal(1993, 1, 1)));
  filters[2].push_back(
      Lt(VirtColRef(scan_schema, "o_orderdate"), Literal(1994, 1, 1)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"o_custkey", "o_orderkey"});
  return std::make_unique<SimdScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["orders"],
      std::move(filters));
}

// region_nation_customer JOIN orders ON c_custkey = o_custkey
std::unique_ptr<Operator> RegionNationCustomerOrders() {
  auto region_nation_customer = RegionNationCustomer();
  auto orders = SelectOrders();

  auto c_custkey = ColRef(region_nation_customer, "c_custkey", 0);
  auto o_custkey = ColRef(orders, "o_custkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region_nation_customer,
                               {"n_name", "n_nationkey"}, 0);
  schema.AddPassthroughColumns(*orders, {"o_orderkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation_customer), std::move(orders),
      util::MakeVector(std::move(c_custkey)),
      util::MakeVector(std::move(o_custkey)));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_extendedprice", "l_discount",
                                              "l_orderkey", "l_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// region_nation_customer_orders JOIN lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> RegionNationCustomerOrdersLineitem() {
  auto rnco = RegionNationCustomerOrders();
  auto lineitem = ScanLineitem();

  auto o_orderkey = ColRef(rnco, "o_orderkey", 0);
  auto l_orderkey = ColRef(lineitem, "l_orderkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*rnco, {"n_name", "n_nationkey"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_extendedprice", "l_discount", "l_suppkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rnco), std::move(lineitem),
      util::MakeVector(std::move(o_orderkey)),
      util::MakeVector(std::move(l_orderkey)));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// supplier JOIN rncol ON s_nationkey = n_nationkey and s_suppkey = l_suppkey
std::unique_ptr<Operator> RegionNationCustomerOrdersLineitemSupplier() {
  auto supplier = ScanSupplier();
  auto rncol = RegionNationCustomerOrdersLineitem();

  auto s_nationkey = ColRef(supplier, "s_nationkey", 0);
  auto n_nationkey = ColRef(rncol, "n_nationkey", 1);
  auto s_suppkey = ColRef(supplier, "s_suppkey", 0);
  auto l_suppkey = ColRef(rncol, "l_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*rncol,
                               {"n_name", "l_extendedprice", "l_discount"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(supplier), std::move(rncol),
      util::MakeVector(std::move(s_nationkey), std::move(s_suppkey)),
      util::MakeVector(std::move(n_nationkey), std::move(l_suppkey)));
}

// Group By n_name -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = RegionNationCustomerOrdersLineitemSupplier();

  // Group by
  auto n_name = ColRefE(base, "n_name");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("n_name", VirtColRef(n_name, 0));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 1));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(n_name)),
      util::MakeVector(std::move(revenue)));
}

// Order By revenue desc
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto revenue = ColRef(agg, "revenue");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(std::move(schema), std::move(agg),
                                           util::MakeVector(std::move(revenue)),
                                           std::vector<bool>{false});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q05) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q05_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "stack_spill"}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "linear_scan"}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, TPCHTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));
