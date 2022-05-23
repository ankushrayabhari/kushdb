#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "benchmark/jcch10/schema.h"
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

// Select(c_mktsegment = 'FURNITURE')
std::unique_ptr<Operator> SelectCustomer() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["customer"],
                                  {"c_mktsegment", "c_custkey"});

  auto eq =
      Exp(Eq(VirtColRef(scan_schema, "c_mktsegment"), Literal("FURNITURE"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"c_custkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["customer"],
      util::MakeVector(std::move(eq)));
}

// Select(o_orderdate < '1993-05-31')
std::unique_ptr<Operator> SelectOrders() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["orders"],
      {"o_orderdate", "o_shippriority", "o_custkey", "o_orderkey"});

  auto lt =
      Exp(Lt(VirtColRef(scan_schema, "o_orderdate"), Literal(1993, 5, 31)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema,
      {"o_orderdate", "o_shippriority", "o_custkey", "o_orderkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["orders"],
      util::MakeVector(std::move(lt)));
}

// Select(l_shipdate > '1993-05-31')
std::unique_ptr<Operator> SelectLineitem() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(
      db["lineitem"],
      {"l_orderkey", "l_extendedprice", "l_shipdate", "l_discount"});

  auto gt =
      Exp(Gt(VirtColRef(scan_schema, "l_shipdate"), Literal(1993, 5, 31)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(
      scan_schema, {"l_orderkey", "l_extendedprice", "l_discount"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["lineitem"],
      util::MakeVector(std::move(gt)));
}

// customers
// JOIN orders ON c_custkey = o_custkey
// JOIN lineitem ON l_orderkey = o_orderkey
std::unique_ptr<Operator> CustomerOrdersLineitem() {
  auto customer = SelectCustomer();
  auto orders = SelectOrders();
  auto lineitem = SelectLineitem();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(customer, "c_custkey", 0), ColRef(orders, "o_custkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 2), ColRef(orders, "o_orderkey", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*orders, {"o_orderdate", "o_shippriority"}, 1);
  schema.AddPassthroughColumns(
      *lineitem, {"l_orderkey", "l_extendedprice", "l_discount"}, 2);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(customer), std::move(orders),
                       std::move(lineitem)),
      std::move(conditions));
}

// Group By l_orderkey, o_orderdate, o_shippriority -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = CustomerOrdersLineitem();

  // Group by
  auto l_orderkey = ColRefE(base, "l_orderkey");
  auto o_orderdate = ColRefE(base, "o_orderdate");
  auto o_shippriority = ColRefE(base, "o_shippriority");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("l_orderkey", VirtColRef(l_orderkey, 0));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 3));
  schema.AddDerivedColumn("o_orderdate", VirtColRef(o_orderdate, 1));
  schema.AddDerivedColumn("o_shippriority", VirtColRef(o_shippriority, 2));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(l_orderkey), std::move(o_orderdate),
                       std::move(o_shippriority)),
      util::MakeVector(std::move(revenue)));
}

// Order By revenue desc, o_orderdate
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto revenue = ColRef(agg, "revenue");
  auto o_orderdate = ColRef(agg, "o_orderdate");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(revenue), std::move(o_orderdate)),
      std::vector<bool>{false, true});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
