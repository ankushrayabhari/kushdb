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
#include "end_to_end_test/test_util.h"
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

// lineitem JOIN part ON l_orderkey = o_orderkey
std::unique_ptr<Operator> LineitemPart() {
  auto lineitem = SelectLineitem();
  auto part = ScanPart();

  auto l_partkey = ColRef(lineitem, "l_partkey", 0);
  auto p_partkey = ColRef(part, "p_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"}, 0);
  schema.AddPassthroughColumns(*part, {"p_type"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(lineitem), std::move(part),
      util::MakeVector(std::move(l_partkey)),
      util::MakeVector(std::move(p_partkey)));
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

TEST_P(TPCHTest, Q14) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(Agg());

  auto expected_file = "end_to_end_test/tpch/q14_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_EQ_TBL(expected, output, query->Child().Schema().Columns(), 1e-5);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "stack_spill"}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "linear_scan"}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, TPCHTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));