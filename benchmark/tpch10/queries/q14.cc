#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "benchmark/tpch10/schema.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
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
#include "util/time_execute.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = Schema();

// Select(l_shipdate >= '1996-03-01' and l_receiptdate < '1996-04-01')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"],
      {"l_shipdate", "l_extendedprice", "l_discount", "l_partkey"});

  auto p1 = Exp(Geq(VirtColRef(scan_schema, "l_shipdate"),
                    Literal(absl::CivilDay(1996, 3, 1))));
  auto p2 = Exp(Lt(VirtColRef(scan_schema, "l_shipdate"),
                   Literal(absl::CivilDay(1996, 4, 1))));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"l_extendedprice", "l_discount", "l_partkey"});
  return std::make_unique<SkinnerScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(p1), std::move(p2)));
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

  std::vector<std::unique_ptr<Expression>> conditions;
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

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(Agg());

  TimeExecute(*query);
  return 0;
}
