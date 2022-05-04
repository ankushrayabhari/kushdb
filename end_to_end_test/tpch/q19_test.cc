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

const Database db = Schema();

// Select(part)
std::unique_ptr<Operator> SelectPart() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["part"], {"p_partkey", "p_brand", "p_container", "p_size"});

  std::unique_ptr<Expression> or1;
  {
    std::unique_ptr<Expression> p1 =
        Geq(VirtColRef(scan_schema, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 =
        Leq(VirtColRef(scan_schema, "p_size"), Literal(5));
    std::unique_ptr<Expression> p3 =
        Eq(VirtColRef(scan_schema, "p_brand"), Literal("Brand#31"sv));
    std::unique_ptr<Expression> p4 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("SM CASE"sv));
    std::unique_ptr<Expression> p5 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("SM BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("SM PACK"sv));
    std::unique_ptr<Expression> p7 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("SM PKG"sv));
    or1 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  std::unique_ptr<Expression> or2;
  {
    std::unique_ptr<Expression> p1 =
        Geq(VirtColRef(scan_schema, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 =
        Leq(VirtColRef(scan_schema, "p_size"), Literal(10));
    std::unique_ptr<Expression> p3 =
        Eq(VirtColRef(scan_schema, "p_brand"), Literal("Brand#52"sv));
    std::unique_ptr<Expression> p4 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("MED BAG"sv));
    std::unique_ptr<Expression> p5 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("MED BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("MED PKG"sv));
    std::unique_ptr<Expression> p7 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("MED PACK"sv));
    or2 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  std::unique_ptr<Expression> or3;
  {
    std::unique_ptr<Expression> p1 =
        Geq(VirtColRef(scan_schema, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 =
        Leq(VirtColRef(scan_schema, "p_size"), Literal(15));
    std::unique_ptr<Expression> p3 =
        Eq(VirtColRef(scan_schema, "p_brand"), Literal("Brand#42"sv));
    std::unique_ptr<Expression> p4 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("LG CASE"sv));
    std::unique_ptr<Expression> p5 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("LG BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("LG PACK"sv));
    std::unique_ptr<Expression> p7 =
        Eq(VirtColRef(scan_schema, "p_container"), Literal("LG PKG"sv));
    or3 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  auto cond =
      Exp(Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3))));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"p_partkey", "p_brand", "p_container", "p_size"});
  return std::make_unique<SkinnerScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["part"],
      util::MakeVector(std::move(cond)));
}

// Select(lineitem)
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"], {"l_extendedprice", "l_discount", "l_quantity",
                       "l_shipmode", "l_shipinstruct", "l_partkey"});

  std::unique_ptr<Expression> and1 =
      Eq(VirtColRef(scan_schema, "l_shipinstruct"),
         Literal("DELIVER IN PERSON"sv));

  std::unique_ptr<Expression> and2;
  {
    std::unique_ptr<Expression> p1 =
        Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("AIR"sv));
    std::unique_ptr<Expression> p2 =
        Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("AIR REG"sv));
    and2 = Or(util::MakeVector(std::move(p1), std::move(p2)));
  }

  std::unique_ptr<Expression> and3;
  {
    std::unique_ptr<Expression> p1 =
        Geq(VirtColRef(scan_schema, "l_quantity"), Literal(3.0));
    std::unique_ptr<Expression> p2 =
        Leq(VirtColRef(scan_schema, "l_quantity"), Literal(13.0));
    std::unique_ptr<Expression> p3 =
        Geq(VirtColRef(scan_schema, "l_quantity"), Literal(12.0));
    std::unique_ptr<Expression> p4 =
        Leq(VirtColRef(scan_schema, "l_quantity"), Literal(22.0));
    std::unique_ptr<Expression> p5 =
        Geq(VirtColRef(scan_schema, "l_quantity"), Literal(25.0));
    std::unique_ptr<Expression> p6 =
        Leq(VirtColRef(scan_schema, "l_quantity"), Literal(35.0));

    std::unique_ptr<Expression> or1 =
        And(util::MakeVector(std::move(p1), std::move(p2)));
    std::unique_ptr<Expression> or2 =
        And(util::MakeVector(std::move(p3), std::move(p4)));
    std::unique_ptr<Expression> or3 =
        And(util::MakeVector(std::move(p5), std::move(p6)));

    and3 = Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3)));
  }

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema,
      {"l_quantity", "l_extendedprice", "l_discount", "l_partkey"});
  return std::make_unique<SkinnerScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(and1), std::move(and2), std::move(and3)));
}

// part JOIN lineitem ON p_partkey = l_partkey
std::unique_ptr<Operator> PartLineitem() {
  auto part = SelectPart();
  auto lineitem = SelectLineitem();

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto l_partkey = ColRef(lineitem, "l_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_brand", "p_container", "p_size"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_quantity", "l_extendedprice", "l_discount"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(lineitem),
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(l_partkey)));
}

// select(join)
std::unique_ptr<Operator> SelectPartLineitem() {
  auto base = PartLineitem();

  std::unique_ptr<Expression> or1;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(5));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#31"sv));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("SM CASE"sv));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("SM BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("SM PACK"sv));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("SM PKG"sv));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(3.0));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(13.0));
    or1 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }

  std::unique_ptr<Expression> or2;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(10));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#52"sv));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("MED BAG"sv));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("MED BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("MED PKG"sv));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("MED PACK"sv));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(12.0));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(22.0));
    or2 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }

  std::unique_ptr<Expression> or3;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(15));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#42"sv));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("LG CASE"sv));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("LG BOX"sv));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("LG PACK"sv));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("LG PKG"sv));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(25.0));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(35.0));
    or3 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }
  auto cond =
      Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*base, {"l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(base),
                                          std::move(cond));
}

// Aggregate
std::unique_ptr<Operator> Agg() {
  auto base = SelectPartLineitem();

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 0));

  return std::make_unique<AggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(revenue)));
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q19) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(Agg());

  auto expected_file = "end_to_end_test/tpch/q19_expected.tbl";
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
