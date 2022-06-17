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
#include "end_to_end_test/test_macros.h"
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
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

const Database db = EnumSchema();

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_extendedprice", "l_discount",
                                              "l_shipdate", "l_quantity"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate >= '1993-01-01' AND l_shipdate < '1994-01-01' AND
// l_discount  >= 0.02 AND l_discount <= 0.04 AND l_quantity < 25)
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> p1 =
        Geq(ColRef(lineitem, "l_shipdate"), Literal(1993, 1, 1));
    std::unique_ptr<Expression> p2 =
        Lt(ColRef(lineitem, "l_shipdate"), Literal(1994, 1, 1));
    std::unique_ptr<Expression> p3 =
        Geq(ColRef(lineitem, "l_discount"), Literal(0.02));
    std::unique_ptr<Expression> p4 =
        Leq(ColRef(lineitem, "l_discount"), Literal(0.04));
    std::unique_ptr<Expression> p5 =
        Lt(ColRef(lineitem, "l_quantity"), Literal(25.0));

    cond = And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                                std::move(p4), std::move(p5)));
  }

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// Agg
std::unique_ptr<Operator> Agg() {
  auto lineitem = SelectLineitem();

  // aggregate
  auto revenue = Sum(
      Mul(ColRef(lineitem, "l_extendedprice"), ColRef(lineitem, "l_discount")));

  OperatorSchema schema;
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 0));

  return std::make_unique<AggregateOperator>(
      std::move(schema), std::move(lineitem),
      util::MakeVector(std::move(revenue)));
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q06) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(Agg());

  auto expected_file = "end_to_end_test/tpch/q06_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

NORMAL_TEST(TPCHTest)
