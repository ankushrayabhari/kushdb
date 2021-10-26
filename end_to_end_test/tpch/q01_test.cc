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
#include "plan/cross_product_operator.h"
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
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

const Database db = Schema();

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(
      db["lineitem"], {"l_returnflag", "l_linestatus", "l_quantity",
                       "l_extendedprice", "l_discount", "l_tax", "l_shipdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select (l_shipdate <= 1998-09-13)
std::unique_ptr<Operator> SelectLineitem() {
  auto scan_lineitem = ScanLineitem();

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
std::unique_ptr<Operator> GroupByAgg() {
  auto select_lineitem = SelectLineitem();

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
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto l_returnflag = ColRef(agg, "l_returnflag");
  auto l_linestatus = ColRef(agg, "l_linestatus");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(l_returnflag), std::move(l_linestatus)),
      std::vector<bool>{true, true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q01) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q01_expected.tbl";
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
