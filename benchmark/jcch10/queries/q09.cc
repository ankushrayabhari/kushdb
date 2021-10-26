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
  schema.AddGeneratedColumns(db["nation"], {"n_name", "n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"], {"p_partkey", "p_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// Select(p_name LIKE '%mined gold%')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> contains =
      Contains(ColRef(part, "p_name"), Literal("hiny mined gold"sv));

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

// nation JOIN supplier ON s_nationkey = n_nationkey
std::unique_ptr<Operator> NationSupplier() {
  auto nation = ScanNation();
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

// part JOIN partsupp ON p_partkey = ps_partkey
std::unique_ptr<Operator> PartPartsupp() {
  auto part = SelectPart();
  auto partsupp = ScanPartsupp();

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto ps_partkey = ColRef(partsupp, "ps_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey"}, 0);
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_suppkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(partsupp),
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(ps_partkey)));
}

// nation_supplier JOIN part_partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> NationSupplierPartPartsupp() {
  auto nation_supp = NationSupplier();
  auto part_partsupp = PartPartsupp();

  auto s_suppkey = ColRef(nation_supp, "s_suppkey", 0);
  auto ps_suppkey = ColRef(part_partsupp, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation_supp, {"s_suppkey", "n_name"}, 0);
  schema.AddPassthroughColumns(*part_partsupp, {"p_partkey", "ps_supplycost"},
                               1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_supp), std::move(part_partsupp),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
}

// nation_supplier_part_partsupp JOIN lineitem ON p_partkey = l_partkey AND
// s_suppkey = l_suppkey
std::unique_ptr<Operator> NationSupplierPartPartsuppLineitem() {
  auto left = NationSupplierPartPartsupp();
  auto lineitem = ScanLineitem();

  auto p_partkey = ColRef(left, "p_partkey", 0);
  auto l_partkey = ColRef(lineitem, "l_partkey", 1);
  auto s_suppkey = ColRef(left, "s_suppkey", 0);
  auto l_suppkey = ColRef(lineitem, "l_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*left, {"ps_supplycost", "n_name"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_extendedprice", "l_discount", "l_quantity", "l_orderkey"},
      1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(left), std::move(lineitem),
      util::MakeVector(std::move(p_partkey), std::move(s_suppkey)),
      util::MakeVector(std::move(l_partkey), std::move(l_suppkey)));
}

// nation_supplier_part_partsupp_lineitem JOIN orders ON o_orderkey = l_orderkey
std::unique_ptr<Operator> Join() {
  auto left = NationSupplierPartPartsuppLineitem();
  auto orders = ScanOrders();

  auto l_orderkey = ColRef(left, "l_orderkey", 0);
  auto o_orderkey = ColRef(orders, "o_orderkey", 1);

  auto o_year = Extract(ColRef(orders, "o_orderdate", 1), ExtractValue::YEAR);
  auto amount =
      Sub(Mul(ColRef(left, "l_extendedprice", 0),
              Sub(Literal(1.0), ColRef(left, "l_discount", 0))),
          Mul(ColRef(left, "ps_supplycost", 0), ColRef(left, "l_quantity", 0)));

  OperatorSchema schema;
  schema.AddPassthroughColumn(*left, "n_name", "nation", 0);
  schema.AddDerivedColumn("o_year", std::move(o_year));
  schema.AddDerivedColumn("amount", std::move(amount));
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(left), std::move(orders),
      util::MakeVector(std::move(l_orderkey)),
      util::MakeVector(std::move(o_orderkey)));
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
