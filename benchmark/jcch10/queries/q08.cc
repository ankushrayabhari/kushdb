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
using namespace std::literals;

Database db;

// Scan(nation)
std::unique_ptr<Operator> ScanNationN1() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_regionkey", "n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(nation)
std::unique_ptr<Operator> ScanNationN2() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Select(r_name = 'AFRICA')
std::unique_ptr<Operator> SelectRegion() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["region"], {"r_name", "r_regionkey"});

  auto eq = Exp(Eq(VirtColRef(scan_schema, "r_name"), Literal("AFRICA"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"r_regionkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["region"],
      util::MakeVector(std::move(eq)));
}

// Select(p_type = 'SHINY MINED GOLD')
std::unique_ptr<Operator> SelectPart() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_type"});

  auto eq =
      Exp(Eq(VirtColRef(scan_schema, "p_type"), Literal("SHINY MINED GOLD"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"p_partkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["part"],
      util::MakeVector(std::move(eq)));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_partkey",
                              "l_suppkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(o_orderdate >= '1993-01-01' and o_orderdate <= '1994-12-31')
std::unique_ptr<Operator> SelectOrders() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["orders"],
                                  {"o_orderkey", "o_custkey", "o_orderdate"});

  auto geq =
      Exp(Geq(VirtColRef(scan_schema, "o_orderdate"), Literal(1993, 1, 1)));
  auto leq =
      Exp(Leq(VirtColRef(scan_schema, "o_orderdate"), Literal(1994, 12, 31)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema);
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["orders"],
      util::MakeVector(std::move(geq), std::move(leq)));
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_custkey", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// part
// JOIN lineitem ON p_partkey = l_partkey
// JOIN supplier ON s_suppkey = l_suppkey
// JOIN orders ON l_orderkey = o_orderkey
// JOIN nation n2 ON s_nationkey = n2.n_nationkey
// JOIN nation n1
// JOIN customer ON c_nationkey = n1.n_nationkey AND o_orderkey = c_custkey
// JOIN region ON n1.n_regionkey = r_regionkey
std::unique_ptr<Operator> Join() {
  auto part = SelectPart();
  auto supplier = ScanSupplier();
  auto lineitem = ScanLineitem();
  auto orders = SelectOrders();
  auto customer = ScanCustomer();
  auto n1 = ScanNationN1();
  auto n2 = ScanNationN2();
  auto region = SelectRegion();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(part, "p_partkey", 0), ColRef(lineitem, "l_partkey", 2)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 1), ColRef(lineitem, "l_suppkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 2), ColRef(orders, "o_orderkey", 3)));
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 3), ColRef(customer, "c_custkey", 4)));
  conditions.push_back(
      Eq(ColRef(customer, "c_nationkey", 4), ColRef(n1, "n_nationkey", 5)));
  conditions.push_back(
      Eq(ColRef(n1, "n_regionkey", 5), ColRef(region, "r_regionkey", 7)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 1), ColRef(n2, "n_nationkey", 6)));

  auto o_year = Extract(ColRef(orders, "o_orderdate", 3), ExtractValue::YEAR);
  auto volume = Mul(ColRef(lineitem, "l_extendedprice", 2),
                    Sub(Literal(1.0), ColRef(lineitem, "l_discount", 2)));

  OperatorSchema schema;
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("volume", std::move(volume));
  schema.AddPassthroughColumn(*n2, "n_name", "nation", 6);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(part), std::move(supplier),
                       std::move(lineitem), std::move(orders),
                       std::move(customer), std::move(n1), std::move(n2),
                       std::move(region)),
      std::move(conditions));
}

// Group By o_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto o_year = ColRefE(base, "o_year");

  // aggregate
  auto kenya_vol = Sum(Case(Eq(ColRef(base, "nation"), Literal("MOROCCO"sv)),
                            ColRef(base, "volume"), Literal(0.0)));
  auto sum_vol = Sum(ColRef(base, "volume"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("o_year", VirtColRef(o_year, 0));
  schema.AddDerivedColumn(
      "mkt_share", Div(VirtColRef(kenya_vol, 1), VirtColRef(sum_vol, 2)));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(o_year)),
      util::MakeVector(std::move(kenya_vol), std::move(sum_vol)));
}

// Order By o_year
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto o_year = ColRef(agg, "o_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);
  return std::make_unique<OrderByOperator>(std::move(schema), std::move(agg),
                                           util::MakeVector(std::move(o_year)),
                                           std::vector<bool>{true});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
