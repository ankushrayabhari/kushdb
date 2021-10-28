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

const Database db = Schema();

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_name", "n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// Select(p_name LIKE '%lawn%')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> contains =
      Contains(ColRef(part, "p_name"), Literal("lawn"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(contains));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_quantity",
                              "l_suppkey", "l_partkey", "l_orderkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Scan(orders)
std::unique_ptr<Operator> ScanOrders() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["orders"], {"o_orderkey", "o_orderdate"});
  return std::make_unique<ScanOperator>(std::move(schema), db["orders"]);
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_suppkey", "ps_partkey", "ps_supplycost"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// supplier
// JOIN lineitem ON s_suppkey = l_suppkey
// JOIN partsupp ON ps_suppkey = l_suppkey AND ps_partkey = l_partkey
// JOIN part ON p_partkey = l_partkey
// JOIN orders ON o_orderkey = l_orderkey
// JOIN nation ON s_nationkey = n_nationkey
std::unique_ptr<Operator> Join() {
  auto supplier = ScanSupplier();
  auto lineitem = ScanLineitem();
  auto partsupp = ScanPartsupp();
  auto part = SelectPart();
  auto orders = ScanOrders();
  auto nation = ScanNation();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 0), ColRef(lineitem, "l_suppkey", 1)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_suppkey", 1), ColRef(partsupp, "ps_suppkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_partkey", 1), ColRef(partsupp, "ps_partkey", 2)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_partkey", 1), ColRef(part, "p_partkey", 3)));
  conditions.push_back(
      Eq(ColRef(lineitem, "l_orderkey", 1), ColRef(orders, "o_orderkey", 4)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_nationkey", 0), ColRef(nation, "n_nationkey", 5)));

  auto o_year = Extract(ColRef(orders, "o_orderdate", 4), ExtractValue::YEAR);
  auto amount = Sub(Mul(ColRef(lineitem, "l_extendedprice", 1),
                        Sub(Literal(1.0), ColRef(lineitem, "l_discount", 1))),
                    Mul(ColRef(partsupp, "ps_supplycost", 2),
                        ColRef(lineitem, "l_quantity", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumn(*nation, "n_name", "nation", 5);
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("amount", std::move(amount));
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(supplier), std::move(lineitem),
                       std::move(partsupp), std::move(part), std::move(orders),
                       std::move(nation)),
      std::move(conditions));
}

// Group By nation, o_year -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = Join();

  // Group by
  auto nation = ColRefE(base, "nation");
  auto o_year = ColRefE(base, "o_year");

  // aggregate
  auto sum_profit = Sum(ColRef(base, "amount"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("nation", VirtColRef(nation, 0));
  schema.AddDerivedColumn("o_year", VirtColRef(o_year, 1));
  schema.AddDerivedColumn("sum_profit", VirtColRef(sum_profit, 2));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(nation), std::move(o_year)),
      util::MakeVector(std::move(sum_profit)));
}

// Order By nation, o_year desc
std::unique_ptr<Operator> OrderBy() {
  auto agg = GroupByAgg();

  auto nation = ColRef(agg, "nation");
  auto o_year = ColRef(agg, "o_year");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);
  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(agg),
      util::MakeVector(std::move(nation), std::move(o_year)),
      std::vector<bool>{true, false});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
