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

// Select((l_shipmode = 'RAIL' OR l_shipmode = 'MAIL') AND l_commitdate <
// l_receiptdate AND l_shipdate < l_commitdate AND l_receiptdate >=
// '1994-01-01' and l_receiptdate < '1995-01-01')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["lineitem"],
                                  {"l_shipmode", "l_commitdate", "l_shipdate",
                                   "l_receiptdate", "l_orderkey"});

  auto or_1 = Exp(Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("RAIL"sv)));
  auto or_2 = Exp(Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("MAIL"sv)));
  auto p1 = Exp(Or(util::MakeVector(std::move(or_1), std::move(or_2))));

  auto p2 = Exp(Lt(VirtColRef(scan_schema, "l_commitdate"),
                   VirtColRef(scan_schema, "l_receiptdate")));
  auto p3 = Exp(Lt(VirtColRef(scan_schema, "l_shipdate"),
                   VirtColRef(scan_schema, "l_commitdate")));
  auto p4 =
      Exp(Geq(VirtColRef(scan_schema, "l_receiptdate"), Literal(1994, 1, 1)));
  auto p5 =
      Exp(Lt(VirtColRef(scan_schema, "l_receiptdate"), Literal(1995, 1, 1)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema,
                                      {"l_shipmode", "l_orderkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                       std::move(p4), std::move(p5)));
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderkey", "o_orderpriority"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// lineitem JOIN orders ON l_orderkey = o_orderkey
std::unique_ptr<Operator> LineitemOrders() {
  auto lineitem = SelectLineitem();
  auto orders = ScanOrders();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 0), ColRef(orders, "o_orderkey", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem, {"l_shipmode"}, 0);
  schema.AddPassthroughColumns(*orders, {"o_orderpriority"}, 1);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(lineitem), std::move(orders)),
      std::move(conditions));
}

// Group By l_shipmode -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = LineitemOrders();

  // Group by
  auto l_shipmode = ColRefE(base, "l_shipmode");

  // aggregate
  std::unique_ptr<Expression> p1 =
      Eq(ColRef(base, "o_orderpriority"), Literal("1-URGENT"sv));
  std::unique_ptr<Expression> p2 =
      Eq(ColRef(base, "o_orderpriority"), Literal("2-HIGH"sv));
  std::unique_ptr<Expression> p3 =
      Neq(ColRef(base, "o_orderpriority"), Literal("1-URGENT"sv));
  std::unique_ptr<Expression> p4 =
      Neq(ColRef(base, "o_orderpriority"), Literal("2-HIGH"sv));

  auto high_line_count =
      Sum(Case(Or(util::MakeVector(std::move(p1), std::move(p2))), Literal(1),
               Literal(0)));
  auto low_line_count =
      Sum(Case(And(util::MakeVector(std::move(p3), std::move(p4))), Literal(1),
               Literal(0)));

  OperatorSchema schema;
  schema.AddDerivedColumn("l_shipmode", VirtColRef(l_shipmode, 0));
  schema.AddDerivedColumn("high_line_count", VirtColRef(high_line_count, 1));
  schema.AddDerivedColumn("low_line_count", VirtColRef(low_line_count, 2));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(l_shipmode)),
      util::MakeVector(std::move(high_line_count), std::move(low_line_count)));
}

// Order By l_shipmode
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto l_shipmode = ColRef(agg, "l_shipmode");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(l_shipmode)), std::vector<bool>{true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q12Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q12_expected.tbl";
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