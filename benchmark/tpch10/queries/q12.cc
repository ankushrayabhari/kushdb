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
#include "plan/operator/simd_scan_select_operator.h"
#include "util/builder.h"
#include "util/time_execute.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

Database db;

// Select((l_shipmode = 'AIR' OR l_shipmode = 'REG AIR') AND l_commitdate <
// l_receiptdate AND l_shipdate < l_commitdate AND l_receiptdate >=
// '1996-01-01' and l_receiptdate < '1997-01-01')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["lineitem"],
                                  {"l_shipmode", "l_commitdate", "l_shipdate",
                                   "l_receiptdate", "l_orderkey"});

  auto or_1 = Exp(Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("AIR"sv)));
  auto or_2 =
      Exp(Eq(VirtColRef(scan_schema, "l_shipmode"), Literal("REG AIR"sv)));
  auto p1 = Exp(Or(util::MakeVector(std::move(or_1), std::move(or_2))));

  auto p2 = Exp(Lt(VirtColRef(scan_schema, "l_commitdate"),
                   VirtColRef(scan_schema, "l_receiptdate")));
  auto p3 = Exp(Lt(VirtColRef(scan_schema, "l_shipdate"),
                   VirtColRef(scan_schema, "l_commitdate")));
  auto p4 =
      Exp(Geq(VirtColRef(scan_schema, "l_receiptdate"), Literal(1996, 1, 1)));
  auto p5 =
      Exp(Lt(VirtColRef(scan_schema, "l_receiptdate"), Literal(1997, 1, 1)));

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

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
