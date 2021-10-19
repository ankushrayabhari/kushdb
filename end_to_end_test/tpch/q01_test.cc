#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/execute_capture.h"
#include "end_to_end_test/parameters.h"
#include "end_to_end_test/schema.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "util/builder.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

const Database db = Schema();

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(
      db["lineitem"], {"l_returnflag", "l_linestatus", "l_quantity",
                       "l_extendedprice", "l_discount", "l_tax", "l_shipdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select (l_shipdate <= 1998-09-13)
std::unique_ptr<Operator> SelectLineitem(const Database& db) {
  auto scan_lineitem = ScanLineitem(db);

  auto leq = std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::LEQ, ColRef(scan_lineitem, "l_shipdate"),
      Literal(absl::CivilDay(1998, 9, 13)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_lineitem,
                               {"l_returnflag", "l_linestatus", "l_quantity",
                                "l_extendedprice", "l_discount", "l_tax"});

  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_lineitem), std::move(leq));
}

// Group By l_returnflag, l_linestatus -> Aggregate
std::unique_ptr<Operator> GroupByAgg(const Database& db) {
  auto select_lineitem = SelectLineitem(db);

  // Group by
  auto l_returnflag = ColRefE(select_lineitem, "l_returnflag");
  auto l_linestatus = ColRefE(select_lineitem, "l_linestatus");

  // aggregate
  auto sum_qty = Sum(ColRef(select_lineitem, "l_quantity"));
  auto sum_base_price = Sum(ColRef(select_lineitem, "l_extendedprice"));
  auto sum_disc_price =
      Sum(Mul(ColRef(select_lineitem, "l_extendedprice"),
              Sub(Literal(1.0), ColRef(select_lineitem, "l_discount"))));
  auto sum_charge =
      Sum(Mul(Mul(ColRef(select_lineitem, "l_extendedprice"),
                  Sub(Literal(1.0), ColRef(select_lineitem, "l_discount"))),
              Add(Literal(1.0), ColRef(select_lineitem, "l_tax"))));
  auto avg_qty = Avg(ColRef(select_lineitem, "l_quantity"));
  auto avg_price = Avg(ColRef(select_lineitem, "l_extendedprice"));
  auto avg_disc = Avg(ColRef(select_lineitem, "l_discount"));
  auto count_order = Count();

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("l_returnflag", VirtColRef(l_returnflag, 0));
  schema.AddDerivedColumn("l_linestatus", VirtColRef(l_linestatus, 1));
  schema.AddDerivedColumn("sum_qty", VirtColRef(sum_qty, 2));
  schema.AddDerivedColumn("sum_base_price", VirtColRef(sum_base_price, 3));
  schema.AddDerivedColumn("sum_disc_price", VirtColRef(sum_disc_price, 4));
  schema.AddDerivedColumn("sum_charge", VirtColRef(sum_charge, 5));
  schema.AddDerivedColumn("avg_qty", VirtColRef(avg_qty, 6));
  schema.AddDerivedColumn("avg_price", VirtColRef(avg_price, 7));
  schema.AddDerivedColumn("avg_disc", VirtColRef(avg_disc, 8));
  schema.AddDerivedColumn("count_order", VirtColRef(count_order, 9));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(select_lineitem),
      util::MakeVector(std::move(l_returnflag), std::move(l_linestatus)),
      util::MakeVector(std::move(sum_qty), std::move(sum_base_price),
                       std::move(sum_disc_price), std::move(sum_charge),
                       std::move(avg_qty), std::move(avg_price),
                       std::move(avg_disc), std::move(count_order)));
}

// Order By l_returnflag, l_linestatus
std::unique_ptr<Operator> OrderBy(const Database& db) {
  auto agg = GroupByAgg(db);

  auto l_returnflag = ColRef(agg, "l_returnflag");
  auto l_linestatus = ColRef(agg, "l_linestatus");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(l_returnflag), std::move(l_linestatus)),
      std::vector<bool>{true, true});
}

std::vector<std::string> Split(const std::string& s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(std::move(item));
  }
  return elems;
}

void EXPECT_EQ_TBL(
    const std::vector<std::string>& expected,
    const std::vector<std::string>& output,
    const std::vector<kush::plan::OperatorSchema::Column>& columns,
    double threshold) {
  EXPECT_EQ(expected.size(), output.size());

  for (int i = 0; i < expected.size(); i++) {
    auto split_e = Split(expected[i], '|');
    auto split_o = Split(output[i], '|');

    EXPECT_EQ(split_e.size(), columns.size());
    EXPECT_EQ(split_o.size(), columns.size());

    for (int j = 0; j < columns.size(); j++) {
      const auto& e_value = split_e[j];
      const auto& o_value = split_o[j];

      switch (columns[j].Expr().Type()) {
        case catalog::SqlType::REAL: {
          double e_value_as_d = std::stod(e_value);
          double o_value_as_d = std::stod(o_value);
          EXPECT_NEAR(e_value_as_d, o_value_as_d, threshold);
          break;
        }

        case catalog::SqlType::SMALLINT:
        case catalog::SqlType::INT:
        case catalog::SqlType::BIGINT:
        case catalog::SqlType::DATE:
        case catalog::SqlType::TEXT:
        case catalog::SqlType::BOOLEAN:
          EXPECT_EQ(e_value, o_value);
          break;
      }
    }
  }
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q01) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy(db));

  auto expected_file = "end_to_end_test/tpch/q01_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_EQ_TBL(expected, output, query->Child().Schema().Columns(), 1e-5);
}
/*
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
        .backend = "llvm", .skinner = "recompile", .budget_per_episode =
10}));*/

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Permute_LowBudget, TPCHTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "permute", .budget_per_episode = 10}));