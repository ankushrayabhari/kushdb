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
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = Schema();

// Scan(nation)
std::unique_ptr<Operator> ScanNationN1() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_regionkey", "n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(nation)
std::unique_ptr<Operator> ScanNationN2() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_name", "r_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

// Select(r_name = 'AFRICA')
std::unique_ptr<Operator> SelectRegion() {
  auto region = ScanRegion();

  std::unique_ptr<Expression> eq =
      Eq(ColRef(region, "r_name"), Literal("AFRICA"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region, {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// Select(p_type = 'LARGE PLATED STEEL')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> eq =
      Eq(ColRef(part, "p_type"), Literal("LARGE PLATED STEEL"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(eq));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_partkey",
                              "l_suppkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"],
                             {"o_orderkey", "o_custkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate >= '1995-01-01' and o_orderdate <= '1996-12-31')
std::unique_ptr<Operator> SelectOrders() {
  auto orders = ScanOrders();

  std::unique_ptr<Expression> geq =
      Geq(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1995, 1, 1)));
  std::unique_ptr<Expression> leq =
      Leq(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1996, 12, 31)));
  std::unique_ptr<Expression> cond =
      And(util::MakeVector(std::move(geq), std::move(leq)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*orders);
  return std::make_unique<SelectOperator>(std::move(schema), std::move(orders),
                                          std::move(cond));
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_custkey", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// part
// JOIN lineitem ON p_partkey = l_partkey
// JOIN supplier ON s_suppkey = l_suppkey
// JOIN orders ON l_orderkey = o_orderkey
// JOIN nation n2 ON s_nationkey = n2.n_nationkey
// JOIN nation n1
// JOIN customer ON c_nationkey = n1.n_nationkey AND o_orderkey = c_custkey
// JOIN region ON n1.n_regionkey = r_regionkey
std::unique_ptr<Operator> Join() {
  auto part = SelectPart();
  auto supplier = ScanSupplier();
  auto lineitem = ScanLineitem();
  auto orders = SelectOrders();
  auto customer = ScanCustomer();
  auto n1 = ScanNationN1();
  auto n2 = ScanNationN2();
  auto region = SelectRegion();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(part, "p_partkey", 0), ColRef(lineitem, "l_partkey", 2)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 1), ColRef(lineitem, "l_suppkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 2), ColRef(orders, "o_orderkey", 3)));
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 3), ColRef(customer, "c_custkey", 4)));
  conditions.push_back(
      Eq(ColRef(customer, "c_nationkey", 4), ColRef(n1, "n_nationkey", 5)));
  conditions.push_back(
      Eq(ColRef(n1, "n_regionkey", 5), ColRef(region, "r_regionkey", 7)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 1), ColRef(n2, "n_nationkey", 6)));

  auto o_year = Extract(ColRef(orders, "o_orderdate", 3), ExtractValue::YEAR);
  auto volume = Mul(ColRef(lineitem, "l_extendedprice", 2),
                    Sub(Literal(1.0), ColRef(lineitem, "l_discount", 2)));

  OperatorSchema schema;
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("volume", std::move(volume));
  schema.AddPassthroughColumn(*n2, "n_name", "nation", 6);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(part), std::move(supplier),
                       std::move(lineitem), std::move(orders),
                       std::move(customer), std::move(n1), std::move(n2),
                       std::move(region)),
      std::move(conditions));
}

// Group By o_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto o_year = ColRefE(base, "o_year");

  // aggregate
  auto kenya_vol = Sum(Case(Eq(ColRef(base, "nation"), Literal("KENYA"sv)),
                            ColRef(base, "volume"), Literal(0.0)));
  auto sum_vol = Sum(ColRef(base, "volume"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("o_year", VirtColRef(o_year, 0));
  schema.AddDerivedColumn(
      "mkt_share", Div(VirtColRef(kenya_vol, 1), VirtColRef(sum_vol, 2)));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(o_year)),
      util::MakeVector(std::move(kenya_vol), std::move(sum_vol)));
}

// Order By o_year
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto o_year = ColRef(agg, "o_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);
  return std::make_unique<OrderByOperator>(std::move(schema), std::move(agg),
                                           util::MakeVector(std::move(o_year)),
                                           std::vector<bool>{true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q08Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q08_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_HighBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_HighBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "permute",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_LowBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "recompile",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_LowBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "permute",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_HighBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_HighBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "permute",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_LowBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "recompile",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_LowBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "permute",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Recompile_HighBudget, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Permute_HighBudget, TPCHTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "permute", .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Recompile_LowBudget, TPCHTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "recompile", .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Permute_LowBudget, TPCHTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "permute", .budget_per_episode = 10}));