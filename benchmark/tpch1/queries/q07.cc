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
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Select(n_name in 'EGYPT' or n_name in 'KENYA')
std::unique_ptr<Operator> SelectNation() {
  auto nation = ScanNation();

  std::unique_ptr<Expression> eq1 =
      Eq(ColRef(nation, "n_name"), Literal("EGYPT"sv));
  std::unique_ptr<Expression> eq2 =
      Eq(ColRef(nation, "n_name"), Literal("KENYA"sv));
  std::unique_ptr<Expression> cond =
      Or(util::MakeVector(std::move(eq1), std::move(eq2)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_nationkey", "n_name"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(nation),
                                          std::move(cond));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
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
  schema.AddGeneratedColumns(db["orders"], {"o_orderkey", "o_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_shipdate", "l_extendedprice", "l_discount",
                              "l_suppkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate >= '1995-01-01' and l_shipdate <= '1996-12-31')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  std::unique_ptr<Expression> ge =
      Geq(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1995, 1, 1)));
  std::unique_ptr<Expression> le = Leq(ColRef(lineitem, "l_shipdate"),
                                       Literal(absl::CivilDay(1996, 12, 31)));
  std::unique_ptr<Expression> cond =
      And(util::MakeVector(std::move(ge), std::move(le)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *lineitem, {"l_shipdate", "l_extendedprice", "l_discount", "l_suppkey",
                  "l_orderkey"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// nation n1 JOIN supplier ON  s_nationkey = n1.n_nationkey
std::unique_ptr<Operator> NationSupplier() {
  auto nation = SelectNation();
  auto supplier = ScanSupplier();

  auto n_nationkey = ColRef(nation, "n_nationkey", 0);
  auto s_nationkey = ColRef(supplier, "s_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name"}, 0);
  schema.AddPassthroughColumns(*supplier, {"s_suppkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation), std::move(supplier),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// nation n2 JOIN customer ON c_nationkey = n2.n_nationkey
std::unique_ptr<Operator> NationCustomer() {
  auto nation = SelectNation();
  auto customer = ScanCustomer();

  auto n_nationkey = ColRef(nation, "n_nationkey", 0);
  auto c_nationkey = ColRef(customer, "c_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name"}, 0);
  schema.AddPassthroughColumns(*customer, {"c_custkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation), std::move(customer),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(c_nationkey)));
}

// nation_customer JOIN orders ON c_custkey = o_custkey
std::unique_ptr<Operator> NationCustomerOrders() {
  auto nation_customer = NationCustomer();
  auto orders = ScanOrders();

  auto c_custkey = ColRef(nation_customer, "c_custkey", 0);
  auto o_custkey = ColRef(orders, "o_custkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation_customer, {"n_name"}, 0);
  schema.AddPassthroughColumns(*orders, {"o_orderkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_customer), std::move(orders),
      util::MakeVector(std::move(c_custkey)),
      util::MakeVector(std::move(o_custkey)));
}

// nation_customer_orders JOIN lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> NationCustomerOrdersLineitem() {
  auto nation_customer_orders = NationCustomerOrders();
  auto lineitem = SelectLineitem();

  auto o_orderkey = ColRef(nation_customer_orders, "o_orderkey", 0);
  auto l_orderkey = ColRef(lineitem, "l_orderkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation_customer_orders, {"n_name"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_shipdate", "l_extendedprice", "l_discount", "l_suppkey"},
      1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_customer_orders), std::move(lineitem),
      util::MakeVector(std::move(o_orderkey)),
      util::MakeVector(std::move(l_orderkey)));
}

// nation_supplier JOIN nation_customer_orders_lineitem ON s_suppkey = l_suppkey
std::unique_ptr<Operator> NationSupplierNationCustomerOrdersLineitem() {
  auto nation_supplier = NationSupplier();
  auto nation_customer_orders_lineitem = NationCustomerOrdersLineitem();

  auto s_suppkey = ColRef(nation_supplier, "s_suppkey", 0);
  auto l_suppkey = ColRef(nation_customer_orders_lineitem, "l_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumn(*nation_supplier, "n_name", "n1_n_name", 0);
  schema.AddPassthroughColumn(*nation_customer_orders_lineitem, "n_name",
                              "n2_n_name", 1);
  schema.AddPassthroughColumns(*nation_customer_orders_lineitem,
                               {"l_shipdate", "l_extendedprice", "l_discount"},
                               1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_supplier),
      std::move(nation_customer_orders_lineitem),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(l_suppkey)));
}

// select(
//  (n1.n_name = 'EGYPT' and n2.n_name = 'KENYA') or
//  (n1.n_name = 'KENYA' and n2.n_name = 'EGYPT')
// )
std::unique_ptr<Operator> SelectNationSupplierNationCustomerOrdersLineitem() {
  auto base = NationSupplierNationCustomerOrdersLineitem();

  std::unique_ptr<Expression> eq1 =
      Eq(ColRef(base, "n1_n_name"), Literal("EGYPT"sv));
  std::unique_ptr<Expression> eq2 =
      Eq(ColRef(base, "n2_n_name"), Literal("KENYA"sv));
  std::unique_ptr<Expression> eq3 =
      Eq(ColRef(base, "n1_n_name"), Literal("KENYA"sv));
  std::unique_ptr<Expression> eq4 =
      Eq(ColRef(base, "n2_n_name"), Literal("EGYPT"sv));

  std::unique_ptr<Expression> or1 =
      And(util::MakeVector(std::move(eq1), std::move(eq2)));
  std::unique_ptr<Expression> or2 =
      And(util::MakeVector(std::move(eq3), std::move(eq4)));
  std::unique_ptr<Expression> cond =
      Or(util::MakeVector(std::move(or1), std::move(or2)));

  auto l_year = Extract(ColRef(base, "l_shipdate"), ExtractValue::YEAR);
  auto volume = Mul(ColRef(base, "l_extendedprice"),
                    Sub(Literal(1.0), ColRef(base, "l_discount")));

  OperatorSchema schema;
  schema.AddPassthroughColumn(*base, "n1_n_name", "supp_nation");
  schema.AddPassthroughColumn(*base, "n2_n_name", "cust_nation");
  schema.AddDerivedColumn("l_year", std::move(l_year));
  schema.AddDerivedColumn("volume", std::move(volume));
  return std::make_unique<SelectOperator>(std::move(schema), std::move(base),
                                          std::move(cond));
}

// Group By supp_nation, cust_nation, l_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = SelectNationSupplierNationCustomerOrdersLineitem();

  // Group by
  auto supp_nation = ColRefE(base, "supp_nation");
  auto cust_nation = ColRefE(base, "cust_nation");
  auto l_year = ColRefE(base, "l_year");

  // aggregate
  auto revenue = Sum(ColRef(base, "volume"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("supp_nation", VirtColRef(supp_nation, 0));
  schema.AddDerivedColumn("cust_nation", VirtColRef(cust_nation, 1));
  schema.AddDerivedColumn("l_year", VirtColRef(l_year, 2));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 3));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(supp_nation), std::move(cust_nation),
                       std::move(l_year)),
      util::MakeVector(std::move(revenue)));
}

// Order By supp_nation, cust_nation, l_year
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto supp_nation = ColRef(agg, "supp_nation");
  auto cust_nation = ColRef(agg, "cust_nation");
  auto l_year = ColRef(agg, "l_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(supp_nation), std::move(cust_nation),
                       std::move(l_year)),
      std::vector<bool>{true, true, true});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(OrderBy());

  BenchVerify(*query, "benchmark/tpch1/raw/q07.tbl");
  return 0;
}
