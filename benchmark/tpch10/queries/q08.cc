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

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_name", "r_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

// Select(r_name = 'AFRICA')
std::unique_ptr<Operator> SelectRegion() {
  auto region = ScanRegion();

  std::unique_ptr<Expression> eq =
      Eq(ColRef(region, "r_name"), Literal("AFRICA"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region, {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// Select(p_type = 'MEDIUM POLISHED COPPER')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> eq =
      Eq(ColRef(part, "p_type"), Literal("MEDIUM POLISHED COPPER"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(eq));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_partkey",
                              "l_suppkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"],
                             {"o_orderkey", "o_custkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate >= '1995-01-01' and o_orderdate <= '1996-12-31')
std::unique_ptr<Operator> SelectOrders() {
  auto orders = ScanOrders();

  std::unique_ptr<Expression> geq =
      Geq(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1995, 1, 1)));
  std::unique_ptr<Expression> leq =
      Leq(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1996, 12, 31)));
  std::unique_ptr<Expression> cond =
      And(util::MakeVector(std::move(geq), std::move(leq)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*orders);
  return std::make_unique<SelectOperator>(std::move(schema), std::move(orders),
                                          std::move(cond));
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

// nation n1 JOIN region ON n1.n_regionkey = r_regionkey
std::unique_ptr<Operator> RegionNationN1() {
  auto region = SelectRegion();
  auto n1 = ScanNationN1();

  auto r_regionkey = ColRef(region, "r_regionkey", 0);
  auto n_regionkey = ColRef(n1, "n_regionkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*n1, {"n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(n1),
      util::MakeVector(std::move(r_regionkey)),
      util::MakeVector(std::move(n_regionkey)));
}

// part JOIN lineitem ON p_partkey = l_partkey
std::unique_ptr<Operator> PartLineitem() {
  auto part = SelectPart();
  auto lineitem = ScanLineitem();

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto l_partkey = ColRef(lineitem, "l_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *lineitem, {"l_extendedprice", "l_discount", "l_suppkey", "l_orderkey"},
      1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(lineitem),
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(l_partkey)));
}

// part_lineitem JOIN orders ON  l_orderkey = o_orderkey
std::unique_ptr<Operator> PartLineitemOrders() {
  auto part_lineitem = PartLineitem();
  auto orders = SelectOrders();

  auto l_orderkey = ColRef(part_lineitem, "l_orderkey", 0);
  auto o_orderkey = ColRef(orders, "o_orderkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *part_lineitem, {"l_extendedprice", "l_discount", "l_suppkey"}, 0);
  schema.AddPassthroughColumns(*orders, {"o_custkey", "o_orderdate"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part_lineitem), std::move(orders),
      util::MakeVector(std::move(l_orderkey)),
      util::MakeVector(std::move(o_orderkey)));
}

// part_lineitem_orders JOIN customer ON o_custkey = c_custkey
std::unique_ptr<Operator> PartLineitemOrdersCustomer() {
  auto part_lineitem_orders = PartLineitemOrders();
  auto customer = ScanCustomer();

  auto o_custkey = ColRef(part_lineitem_orders, "o_custkey", 0);
  auto c_custkey = ColRef(customer, "c_custkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *part_lineitem_orders,
      {"l_extendedprice", "l_discount", "l_suppkey", "o_orderdate"}, 0);
  schema.AddPassthroughColumns(*customer, {"c_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part_lineitem_orders), std::move(customer),
      util::MakeVector(std::move(o_custkey)),
      util::MakeVector(std::move(c_custkey)));
}

// region_nation1 JOIN part_lineitem_orders_customer ON c_nationkey =
// n1.n_nationkey
std::unique_ptr<Operator> RegionNationN1PartLineitemOrdersCustomer() {
  auto region_nation1 = RegionNationN1();
  auto part_lineitem_orders_customer = PartLineitemOrdersCustomer();

  auto o_custkey = ColRef(region_nation1, "n_nationkey", 0);
  auto c_nationkey = ColRef(part_lineitem_orders_customer, "c_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *part_lineitem_orders_customer,
      {"l_extendedprice", "l_discount", "l_suppkey", "o_orderdate"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation1),
      std::move(part_lineitem_orders_customer),
      util::MakeVector(std::move(o_custkey)),
      util::MakeVector(std::move(c_nationkey)));
}

// region_nation1_part_lineitem_orders_customer JOIN supplier ON l_suppkey =
// s_suppkey
std::unique_ptr<Operator> RegionNationN1PartLineitemOrdersCustomerSupplier() {
  auto left = RegionNationN1PartLineitemOrdersCustomer();
  auto supp = ScanSupplier();

  auto l_suppkey = ColRef(left, "l_suppkey", 0);
  auto s_suppkey = ColRef(supp, "s_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *left, {"l_extendedprice", "l_discount", "o_orderdate"}, 0);
  schema.AddPassthroughColumns(*supp, {"s_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(left), std::move(supp),
      util::MakeVector(std::move(l_suppkey)),
      util::MakeVector(std::move(s_suppkey)));
}

// nation n2 JOIN region_nation1_part_lineitem_orders_customer_supplier ON
// n2.n_nationkey = s_nationkey
std::unique_ptr<Operator> Join() {
  auto n2 = ScanNationN2();
  auto right = RegionNationN1PartLineitemOrdersCustomerSupplier();

  auto n_nationkey = ColRef(n2, "n_nationkey", 0);
  auto s_nationkey = ColRef(right, "s_nationkey", 1);

  auto o_year = Extract(ColRef(right, "o_orderdate", 1), ExtractValue::YEAR);
  auto volume = Mul(ColRef(right, "l_extendedprice", 1),
                    Sub(Literal(1.0), ColRef(right, "l_discount", 1)));

  OperatorSchema schema;
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("volume", std::move(volume));
  schema.AddPassthroughColumn(*n2, "n_name", "nation", 0);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(n2), std::move(right),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// Group By o_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto o_year = ColRefE(base, "o_year");

  // aggregate
  auto kenya_vol = Sum(Case(Eq(ColRef(base, "nation"), Literal("ALGERIA"sv)),
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
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(OrderBy());

  kush::util::ExecuteAndTime(*query);
  return 0;
}
