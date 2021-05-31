#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

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
#include "tpch/queries/builder.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = Schema();

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_mktsegment", "c_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Select(c_mktsegment = 'FURNITURE')
std::unique_ptr<Operator> SelectCustomer() {
  auto scan_customer = ScanCustomer();

  auto eq = Eq(ColRef(scan_customer, "c_mktsegment"), Literal("FURNITURE"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_customer, {"c_custkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_customer), std::move(eq));
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderdate", "o_shippriority",
                                            "o_custkey", "o_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate < '1995-03-29')
std::unique_ptr<Operator> SelectOrders() {
  auto scan_orders = ScanOrders();
  auto lt = Lt(ColRef(scan_orders, "o_orderdate"),
               Literal(absl::CivilDay(1995, 3, 29)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_orders, {"o_orderdate", "o_shippriority",
                                              "o_custkey", "o_orderkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_orders), std::move(lt));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_extendedprice",
                                              "l_shipdate", "l_discount"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate > '1995-03-29')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();
  auto gt =
      Gt(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1995, 3, 29)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem,
                               {"l_orderkey", "l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(gt));
}

// customers
// JOIN orders ON c_custkey = o_custkey
// JOIN lineitem ON l_orderkey = o_orderkey
std::unique_ptr<Operator> CustomerOrdersLineitem() {
  auto customer = SelectCustomer();
  auto orders = SelectOrders();
  auto lineitem = SelectLineitem();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
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

int main() {
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(OrderBy());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Execute();
  return 0;
}
