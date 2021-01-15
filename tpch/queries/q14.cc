#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
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
#include "tpch/queries/builder.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
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
  return std::make_unique<ScanOperator>(std::move(schema), "lineitem");
}

// Select(l_shipdate >= '1994-03-01' and l_receiptdate < '1994-04-01')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  std::unique_ptr<Expression> p1 =
      Geq(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1994, 3, 1)));
  std::unique_ptr<Expression> p2 =
      Lt(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1994, 4, 1)));
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
  return std::make_unique<ScanOperator>(std::move(schema), "part");
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
                           Sub(Literal(1), ColRef(base, "l_discount"))),
                       Literal(0.0)));
  auto agg2 = Sum(Mul(ColRef(base, "l_extendedprice"),
                      Sub(Literal(1), ColRef(base, "l_discount"))));

  OperatorSchema schema;
  schema.AddDerivedColumn(
      "promo_revenue",
      Div(Mul(Literal(100.0), VirtColRef(agg1, 0)), VirtColRef(agg2, 1)));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      std::vector<std::unique_ptr<Expression>>(),
      util::MakeVector(std::move(agg1), std::move(agg2)));
}

int main() {
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(Agg());

  QueryTranslator translator(db, *query);
  auto prog = translator.Translate();
  prog.Compile();
  prog.Execute();
  return 0;
}
