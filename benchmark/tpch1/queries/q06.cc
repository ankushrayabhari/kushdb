#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "benchmark/tpch1/schema.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/aggregate_operator.h"
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
#include "util/time_execute.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

Database db;
// Select(l_shipdate >= '1993-01-01' AND l_shipdate < '1994-01-01' AND
// l_discount  >= 0.02 AND l_discount <= 0.04 AND l_quantity < 25)
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"],
      {"l_extendedprice", "l_discount", "l_shipdate", "l_quantity"});

  auto p1 =
      Exp(Geq(VirtColRef(scan_schema, "l_shipdate"), Literal(1993, 1, 1)));
  auto p2 = Exp(Lt(VirtColRef(scan_schema, "l_shipdate"), Literal(1994, 1, 1)));
  auto p3 = Exp(Geq(VirtColRef(scan_schema, "l_discount"), Literal(0.02)));
  auto p4 = Exp(Leq(VirtColRef(scan_schema, "l_discount"), Literal(0.04)));
  auto p5 = Exp(Lt(VirtColRef(scan_schema, "l_quantity"), Literal(25.0)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema,
                                      {"l_extendedprice", "l_discount"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                       std::move(p4), std::move(p5)));
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

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(Agg());

  TimeExecute(*query);
  return 0;
}
