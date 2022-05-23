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

const Database db = Schema();

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_name", "n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Select(p_name LIKE '%moccasin%')
std::unique_ptr<Operator> SelectPart() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_name"});

  auto cond =
      Exp(Contains(VirtColRef(scan_schema, "p_name"), Literal("moccasin"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"p_partkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["part"],
      util::MakeVector(std::move(cond)));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_quantity",
                              "l_suppkey", "l_partkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_suppkey", "ps_partkey", "ps_supplycost"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// supplier
// JOIN lineitem ON s_suppkey = l_suppkey
// JOIN partsupp ON ps_suppkey = l_suppkey AND ps_partkey = l_partkey
// JOIN part ON p_partkey = l_partkey
// JOIN orders ON o_orderkey = l_orderkey
// JOIN nation ON s_nationkey = n_nationkey
std::unique_ptr<Operator> Join() {
  auto supplier = ScanSupplier();
  auto lineitem = ScanLineitem();
  auto partsupp = ScanPartsupp();
  auto part = SelectPart();
  auto orders = ScanOrders();
  auto nation = ScanNation();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 0), ColRef(lineitem, "l_suppkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_suppkey", 1), ColRef(partsupp, "ps_suppkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_partkey", 1), ColRef(partsupp, "ps_partkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_partkey", 1), ColRef(part, "p_partkey", 3)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 1), ColRef(orders, "o_orderkey", 4)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 0), ColRef(nation, "n_nationkey", 5)));

  auto o_year = Extract(ColRef(orders, "o_orderdate", 4), ExtractValue::YEAR);
  auto amount = Sub(Mul(ColRef(lineitem, "l_extendedprice", 1),
                        Sub(Literal(1.0), ColRef(lineitem, "l_discount", 1))),
                    Mul(ColRef(partsupp, "ps_supplycost", 2),
                        ColRef(lineitem, "l_quantity", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumn(*nation, "n_name", "nation", 5);
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("amount", std::move(amount));
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(supplier), std::move(lineitem),
                       std::move(partsupp), std::move(part), std::move(orders),
                       std::move(nation)),
      std::move(conditions));
}

// Group By nation, o_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto nation = ColRefE(base, "nation");
  auto o_year = ColRefE(base, "o_year");

  // aggregate
  auto sum_profit = Sum(ColRef(base, "amount"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("nation", VirtColRef(nation, 0));
  schema.AddDerivedColumn("o_year", VirtColRef(o_year, 1));
  schema.AddDerivedColumn("sum_profit", VirtColRef(sum_profit, 2));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(nation), std::move(o_year)),
      util::MakeVector(std::move(sum_profit)));
}

// Order By nation, o_year desc
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto nation = ColRef(agg, "nation");
  auto o_year = ColRef(agg, "o_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);
  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(nation), std::move(o_year)),
      std::vector<bool>{true, false});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q09Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q09_expected.tbl";
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