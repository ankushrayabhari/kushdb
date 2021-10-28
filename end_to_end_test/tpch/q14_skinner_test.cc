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

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_shipdate", "l_extendedprice",
                                              "l_discount", "l_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate >= '1995-12-01' and l_receiptdate < '1996-01-01')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  std::unique_ptr<Expression> p1 =
      Geq(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1995, 12, 1)));
  std::unique_ptr<Expression> p2 =
      Lt(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1996, 1, 1)));
  auto cond = And(util::MakeVector(std::move(p1), std::move(p2)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem,
                               {"l_extendedprice", "l_discount", "l_partkey"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"], {"p_type", "p_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// lineitem JOIN part ON l_partkey = p_partkey
std::unique_ptr<Operator> LineitemPart() {
  auto lineitem = SelectLineitem();
  auto part = ScanPart();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(lineitem, "l_partkey", 0), ColRef(part, "p_partkey", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"}, 0);
  schema.AddPassthroughColumns(*part, {"p_type"}, 1);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema), util::MakeVector(std::move(lineitem), std::move(part)),
      std::move(conditions));
}

// Aggregate
std::unique_ptr<Operator> Agg() {
  auto base = LineitemPart();

  // aggregate
  auto agg1 = Sum(Case(StartsWith(ColRef(base, "p_type"), Literal("PROMO"sv)),
                       Mul(ColRef(base, "l_extendedprice"),
                           Sub(Literal(1.0), ColRef(base, "l_discount"))),
                       Literal(0.0)));
  auto agg2 = Sum(Mul(ColRef(base, "l_extendedprice"),
                      Sub(Literal(1.0), ColRef(base, "l_discount"))));

  OperatorSchema schema;
  schema.AddDerivedColumn(
      "promo_revenue",
      Div(Mul(Literal(100.0), VirtColRef(agg1, 0)), VirtColRef(agg2, 1)));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      std::vector<std::unique_ptr<Expression>>(),
      util::MakeVector(std::move(agg1), std::move(agg2)));
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q14Skinner) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(Agg());

  auto expected_file = "end_to_end_test/tpch/q14_expected.tbl";
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