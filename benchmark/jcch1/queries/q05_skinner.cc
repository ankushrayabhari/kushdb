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

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_name", "r_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion() {
  auto region = ScanRegion();

  auto eq = Eq(ColRef(region, "r_name"), Literal("MIDDLE EAST"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region, {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_nationkey", "n_regionkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_custkey", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"],
                             {"o_custkey", "o_orderkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_extendedprice", "l_discount",
                                              "l_orderkey", "l_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(o_orderdate >= date '1993-01-01' and o_orderdate < date '1994-01-01')
std::unique_ptr<Operator> SelectOrders() {
  auto orders = ScanOrders();

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> geq =
        Geq(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1993, 1, 1)));
    std::unique_ptr<Expression> lt =
        Lt(ColRef(orders, "o_orderdate"), Literal(absl::CivilDay(1994, 1, 1)));

    cond = And(util::MakeVector(std::move(geq), std::move(lt)));
  }

  OperatorSchema schema;
  schema.AddPassthroughColumns(*orders, {"o_custkey", "o_orderkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(orders),
                                          std::move(cond));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// region
// JOIN nation ON r_regionkey = n_regionkey
// JOIN customer ON n_nationkey = c_nationkey
// JOIN orders ON c_custkey = o_custkey
// JOIN lineitem ON o_orderkey = l_orderkey
// JOIN supplier ON s_nationkey = n_nationkey and s_suppkey = l_suppkey
std::unique_ptr<Operator> RegionNationCustomerOrdersLineitemSupplier() {
  auto region = SelectRegion();
  auto nation = ScanNation();
  auto customer = ScanCustomer();
  auto orders = SelectOrders();
  auto lineitem = ScanLineitem();
  auto supplier = ScanSupplier();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(region, "r_regionkey", 0), ColRef(nation, "n_regionkey", 1)));
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 1), ColRef(customer, "c_nationkey", 2)));
  conditions.push_back(
      Eq(ColRef(customer, "c_custkey", 2), ColRef(orders, "o_custkey", 3)));
  conditions.push_back(
      Eq(ColRef(orders, "o_orderkey", 3), ColRef(lineitem, "l_orderkey", 4)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 5), ColRef(nation, "n_nationkey", 1)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 5), ColRef(lineitem, "l_suppkey", 4)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name"}, 1);
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"}, 4);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(region), std::move(nation),
                       std::move(customer), std::move(orders),
                       std::move(lineitem), std::move(supplier)),
      std::move(conditions));
}

// Group By n_name -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = RegionNationCustomerOrdersLineitemSupplier();

  // Group by
  auto n_name = ColRefE(base, "n_name");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("n_name", VirtColRef(n_name, 0));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 1));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(n_name)),
      util::MakeVector(std::move(revenue)));
}

// Order By revenue desc
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto revenue = ColRef(agg, "revenue");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(std::move(schema), std::move(agg),
                                           util::MakeVector(std::move(revenue)),
                                           std::vector<bool>{false});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(OrderBy());

  BenchVerify(*query, "benchmark/jcch1/raw/q05.tbl");
  return 0;
}
