#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "plan/cross_product_operator.h"
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

const Database db = Schema();

// Scan(lineitem)
std::unique_ptr<Operator> SubqueryScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_quantity"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Group By l_orderkey -> sum(l_quantity)
std::unique_ptr<Operator> SubqueryAgg() {
  auto lineitem = SubqueryScanLineitem();

  auto l_orderkey = ColRefE(lineitem, "l_orderkey");

  auto sum_l_quantity = Sum(ColRef(lineitem, "l_quantity"));

  OperatorSchema schema;
  schema.AddDerivedColumn("l_orderkey", VirtColRef(l_orderkey, 0));
  schema.AddDerivedColumn("sum_l_quantity", VirtColRef(sum_l_quantity, 1));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(lineitem),
      util::MakeVector(std::move(l_orderkey)),
      util::MakeVector(std::move(sum_l_quantity)));
}

// Select(sum(l_quantity) > 313)
std::unique_ptr<Operator> SubquerySelect() {
  auto subquery = SubqueryAgg();

  std::unique_ptr<Expression> cond =
      Gt(ColRef(subquery, "sum_l_quantity"), Literal(313.0));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*subquery, {"l_orderkey"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(subquery), std::move(cond));
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(
      db["orders"], {"o_orderkey", "o_orderdate", "o_totalprice", "o_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"], {"c_name", "c_custkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLinetem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_quantity"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// subquery
// JOIN orders ON l_orderkey = o_orderkey
// JOIN customer ON o_custkey = c_custkey
// JOIN lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> SubqueryOrdersCustomerLineitem() {
  auto subquery = SubquerySelect();
  auto orders = ScanOrders();
  auto customer = ScanCustomer();
  auto lineitem = ScanLinetem();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(subquery, "l_orderkey", 0), ColRef(orders, "o_orderkey", 1)));
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 1), ColRef(customer, "c_custkey", 2)));
  conditions.push_back(
      Eq(ColRef(orders, "o_orderkey", 1), ColRef(lineitem, "l_orderkey", 3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *orders, {"o_orderkey", "o_orderdate", "o_totalprice"}, 1);
  schema.AddPassthroughColumns(*customer, {"c_name", "c_custkey"}, 2);
  schema.AddPassthroughColumns(*lineitem, {"l_quantity"}, 3);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(subquery), std::move(orders),
                       std::move(customer), std::move(lineitem)),
      std::move(conditions));
}

// Group By Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = SubqueryOrdersCustomerLineitem();

  // Group by
  auto c_name = ColRefE(base, "c_name");
  auto c_custkey = ColRefE(base, "c_custkey");
  auto o_orderkey = ColRefE(base, "o_orderkey");
  auto o_orderdate = ColRefE(base, "o_orderdate");
  auto o_totalprice = ColRefE(base, "o_totalprice");

  // aggregate
  auto revenue = Sum(ColRef(base, "l_quantity"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("c_name", VirtColRef(c_name, 0));
  schema.AddDerivedColumn("c_custkey", VirtColRef(c_custkey, 1));
  schema.AddDerivedColumn("o_orderkey", VirtColRef(o_orderkey, 2));
  schema.AddDerivedColumn("o_orderdate", VirtColRef(o_orderdate, 3));
  schema.AddDerivedColumn("o_totalprice", VirtColRef(o_totalprice, 4));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 5));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(c_name), std::move(c_custkey),
                       std::move(o_orderkey), std::move(o_orderdate),
                       std::move(o_totalprice)),
      util::MakeVector(std::move(revenue)));
}

// Order By o_totalprice desc, o_orderdate
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto o_totalprice = ColRef(agg, "o_totalprice");
  auto o_orderdate = ColRef(agg, "o_orderdate");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);
  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(o_totalprice), std::move(o_orderdate)),
      std::vector<bool>{false, true});
}

int main() {
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(OrderBy());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Execute();
  return 0;
}
