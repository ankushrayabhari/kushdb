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
#include "plan/operator/skinner_scan_select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = EnumSchema();

// Select(o_orderdate >= '1995-01-01' and o_orderdate < '1995-04-01')
std::unique_ptr<Operator> SelectOrders() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["orders"],
                                  {"o_custkey", "o_orderkey", "o_orderdate"});

  auto geq =
      Exp(Geq(VirtColRef(scan_schema, "o_orderdate"), Literal(1995, 1, 1)));
  auto lt =
      Exp(Lt(VirtColRef(scan_schema, "o_orderdate"), Literal(1995, 4, 1)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"o_custkey", "o_orderkey"});
  return std::make_unique<SkinnerScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["orders"],
      util::MakeVector(std::move(geq), std::move(lt)));
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"],
                             {"c_custkey", "c_name", "c_acctbal", "c_address",
                              "c_phone", "c_comment", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Select(l_returnflag = 'R')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"],
      {"l_orderkey", "l_returnflag", "l_extendedprice", "l_discount"});

  auto cond = Exp(Eq(VirtColRef(scan_schema, "l_returnflag"), Literal("R"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"l_orderkey", "l_extendedprice", "l_discount"});
  return std::make_unique<SkinnerScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(cond)));
}

// orders
// JOIN customer ON o_custkey = c_custkey
// JOIN nation ON n_nationkey = c_nationkey
// JOIN lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> NationOrdersCustomerLineitem() {
  auto orders = SelectOrders();
  auto customer = ScanCustomer();
  auto nation = ScanNation();
  auto lineitem = SelectLineitem();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 0), ColRef(customer, "c_custkey", 1)));
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 2), ColRef(customer, "c_nationkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 3), ColRef(orders, "o_orderkey", 0)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *customer,
      {"c_custkey", "c_name", "c_acctbal", "c_address", "c_phone", "c_comment"},
      1);
  schema.AddPassthroughColumns(*nation, {"n_name"}, 2);
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"}, 3);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(orders), std::move(customer),
                       std::move(nation), std::move(lineitem)),
      std::move(conditions));
}

// Group By c_custkey, c_name, c_acctbal, c_phone, n_name, c_address, c_comment
std::unique_ptr<Operator> GroupByAgg() {
  auto base = NationOrdersCustomerLineitem();

  // Group by
  auto c_custkey = ColRefE(base, "c_custkey");
  auto c_name = ColRefE(base, "c_name");
  auto c_acctbal = ColRefE(base, "c_acctbal");
  auto c_phone = ColRefE(base, "c_phone");
  auto n_name = ColRefE(base, "n_name");
  auto c_address = ColRefE(base, "c_address");
  auto c_comment = ColRefE(base, "c_comment");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("c_custkey", VirtColRef(c_custkey, 0));
  schema.AddDerivedColumn("c_name", VirtColRef(c_name, 1));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 7));
  schema.AddDerivedColumn("c_acctbal", VirtColRef(c_acctbal, 2));
  schema.AddDerivedColumn("n_name", VirtColRef(n_name, 4));
  schema.AddDerivedColumn("c_address", VirtColRef(c_address, 5));
  schema.AddDerivedColumn("c_phone", VirtColRef(c_phone, 3));
  schema.AddDerivedColumn("c_comment", VirtColRef(c_comment, 6));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(c_custkey), std::move(c_name),
                       std::move(c_acctbal), std::move(c_phone),
                       std::move(n_name), std::move(c_address),
                       std::move(c_comment)),
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

TEST_P(TPCHTest, Q10Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q10_expected.tbl";
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