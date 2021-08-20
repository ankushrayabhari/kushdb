#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
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
#include "tpch/schema.h"
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

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"],
                             {"o_orderpriority", "o_orderdate", "o_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate >= '1997-09-01' and o_orderdate < timestamp '1997-12-01')
std::unique_ptr<Operator> SelectOrders() {
  auto scan = ScanOrders();

  std::unique_ptr<Expression> c1 =
      Geq(ColRef(scan, "o_orderdate"), Literal(absl::CivilDay(1997, 9, 1)));
  std::unique_ptr<Expression> c2 =
      Lt(ColRef(scan, "o_orderdate"), Literal(absl::CivilDay(1997, 12, 1)));
  auto cond = And(util::MakeVector(std::move(c1), std::move(c2)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan, {"o_orderpriority", "o_orderkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(scan),
                                          std::move(cond));
}

// Lineitem
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_orderkey", "l_commitdate", "l_receiptdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_commitdate < l_receiptdate)
std::unique_ptr<Operator> SelectLineitem() {
  auto scan_lineitem = ScanLineitem();
  auto lt = Lt(ColRef(scan_lineitem, "l_commitdate"),
               ColRef(scan_lineitem, "l_receiptdate"));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_lineitem, {"l_orderkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_lineitem), std::move(lt));
}

// select_orders JOIN select_lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> OrdersLineitem() {
  auto orders = SelectOrders();
  auto lineitem = SelectLineitem();

  auto o_orderkey = ColRef(orders, "o_orderkey", 0);
  auto l_orderkey = ColRef(lineitem, "l_orderkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*orders, {"o_orderpriority"}, 0);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(orders), std::move(lineitem),
      util::MakeVector(std::move(o_orderkey)),
      util::MakeVector(std::move(l_orderkey)));
}

// Group By l_orderkey, o_orderdate, o_shippriority -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = OrdersLineitem();

  // Group by
  auto o_orderpriority = ColRefE(base, "o_orderpriority");

  // aggregate
  auto order_count = Count();

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("o_orderpriority", VirtColRef(o_orderpriority, 0));
  schema.AddDerivedColumn("order_count", VirtColRef(order_count, 1));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(o_orderpriority)),
      util::MakeVector(std::move(order_count)));
}

// Order By o_orderpriority
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto o_orderpriority = ColRef(agg, "o_orderpriority");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(o_orderpriority)), std::vector<bool>{true});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(OrderBy());

  kush::util::ExecuteAndTime(*query);
  return 0;
}
