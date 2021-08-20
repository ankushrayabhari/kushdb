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
                             {"o_custkey", "o_orderkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Select(o_orderdate >= '1995-01-01' and o_orderdate < '1995-04-01')
std::unique_ptr<Operator> SelectOrders() {
  auto scan_orders = ScanOrders();

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> geq = Geq(ColRef(scan_orders, "o_orderdate"),
                                          Literal(absl::CivilDay(1995, 1, 1)));
    std::unique_ptr<Expression> lt = Lt(ColRef(scan_orders, "o_orderdate"),
                                        Literal(absl::CivilDay(1995, 4, 1)));
    cond = And(util::MakeVector(std::move(geq), std::move(lt)));
  }

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan_orders, {"o_custkey", "o_orderkey"});
  return std::make_unique<SelectOperator>(
      std::move(schema), std::move(scan_orders), std::move(cond));
}

// Scan(customer)
std::unique_ptr<Operator> ScanCustomer() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["customer"],
                             {"c_custkey", "c_name", "c_acctbal", "c_address",
                              "c_phone", "c_comment", "c_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["customer"]);
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_orderkey", "l_returnflag",
                                              "l_extendedprice", "l_discount"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_returnflag = 'R')
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  auto cond = Eq(ColRef(lineitem, "l_returnflag"), Literal("R"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem,
                               {"l_orderkey", "l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// orders
// JOIN customer ON o_custkey = c_custkey
// JOIN nation ON n_nationkey = c_nationkey
// JOIN lineitem ON o_orderkey = l_orderkey
std::unique_ptr<Operator> NationOrdersCustomerLineitem() {
  auto orders = SelectOrders();
  auto customer = ScanCustomer();
  auto nation = ScanNation();
  auto lineitem = SelectLineitem();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(orders, "o_custkey", 0), ColRef(customer, "c_custkey", 1)));
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 2), ColRef(customer, "c_nationkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 3), ColRef(orders, "o_orderkey", 0)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *customer,
      {"c_custkey", "c_name", "c_acctbal", "c_address", "c_phone", "c_comment"},
      1);
  schema.AddPassthroughColumns(*nation, {"n_name"}, 2);
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"}, 3);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(orders), std::move(customer),
                       std::move(nation), std::move(lineitem)),
      std::move(conditions));
}

// Group By c_custkey, c_name, c_acctbal, c_phone, n_name, c_address, c_comment
std::unique_ptr<Operator> GroupByAgg() {
  auto base = NationOrdersCustomerLineitem();

  // Group by
  auto c_custkey = ColRefE(base, "c_custkey");
  auto c_name = ColRefE(base, "c_name");
  auto c_acctbal = ColRefE(base, "c_acctbal");
  auto c_phone = ColRefE(base, "c_phone");
  auto n_name = ColRefE(base, "n_name");
  auto c_address = ColRefE(base, "c_address");
  auto c_comment = ColRefE(base, "c_comment");

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1.0), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("c_custkey", VirtColRef(c_custkey, 0));
  schema.AddDerivedColumn("c_name", VirtColRef(c_name, 1));
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 7));
  schema.AddDerivedColumn("c_acctbal", VirtColRef(c_acctbal, 2));
  schema.AddDerivedColumn("n_name", VirtColRef(n_name, 4));
  schema.AddDerivedColumn("c_address", VirtColRef(c_address, 5));
  schema.AddDerivedColumn("c_phone", VirtColRef(c_phone, 3));
  schema.AddDerivedColumn("c_comment", VirtColRef(c_comment, 6));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(c_custkey), std::move(c_name),
                       std::move(c_acctbal), std::move(c_phone),
                       std::move(n_name), std::move(c_address),
                       std::move(c_comment)),
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
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(OrderBy());

  kush::util::ExecuteAndTime(*query);
  return 0;
}
