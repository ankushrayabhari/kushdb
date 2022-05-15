#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "benchmark/jcch1/schema.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
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

Database db;

// Select (l_shipdate <= 1998-09-10)
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"], {"l_returnflag", "l_linestatus", "l_quantity",
                       "l_extendedprice", "l_discount", "l_tax", "l_shipdate"});

  auto leq =
      Exp(Leq(VirtColRef(scan_schema, "l_shipdate"), Literal(1998, 9, 10)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"l_returnflag", "l_linestatus", "l_quantity",
                    "l_extendedprice", "l_discount", "l_tax"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(leq)));
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

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
