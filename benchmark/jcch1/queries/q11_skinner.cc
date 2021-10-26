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

// Select(n_name = 'EGYPT')
std::unique_ptr<Operator> SelectNation() {
  auto nation = ScanNation();
  auto eq = Eq(ColRef(nation, "n_name"), Literal("EGYPT"sv));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_nationkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(nation),
                                          std::move(eq));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Scan(partsupp)
std::unique_ptr<Operator> SubqueryScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_supplycost", "ps_availqty", "ps_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// nation
// JOIN supplier ON n_nationkey = s_nationkey
// JOIN partsupp ON ps_suppkey = s_suppkey
std::unique_ptr<Operator> SubqueryNationSupplierPartsupp() {
  auto nation = SelectNation();
  auto supplier = ScanSupplier();
  auto partsupp = SubqueryScanPartsupp();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 0), ColRef(supplier, "s_nationkey", 1)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 1), ColRef(partsupp, "ps_suppkey", 2)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_availqty"}, 2);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(nation), std::move(supplier),
                       std::move(partsupp)),
      std::move(conditions));
}

// Agg
std::unique_ptr<Operator> SubqueryAgg() {
  auto base = SubqueryNationSupplierPartsupp();

  // aggregate
  auto value = Sum(
      Mul(ColRef(base, "ps_supplycost"), Float(ColRef(base, "ps_availqty"))));

  OperatorSchema schema;
  schema.AddDerivedColumn("value1", Mul(VirtColRef(value, 0), Literal(0.0001)));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      std::vector<std::unique_ptr<Expression>>(),
      util::MakeVector(std::move(value)));
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"], {"ps_partkey", "ps_supplycost",
                                              "ps_availqty", "ps_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// nation
// JOIN supplier ON n_nationkey = s_nationkey
// JOIN partsupp ON ps_suppkey = s_suppkey
std::unique_ptr<Operator> NationSupplierPartsupp() {
  auto nation = SelectNation();
  auto supplier = ScanSupplier();
  auto partsupp = ScanPartsupp();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 0), ColRef(supplier, "s_nationkey", 1)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 1), ColRef(partsupp, "ps_suppkey", 2)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *partsupp, {"ps_partkey", "ps_supplycost", "ps_availqty"}, 2);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(nation), std::move(supplier),
                       std::move(partsupp)),
      std::move(conditions));
}

// Group By ps_partkey -> Agg
std::unique_ptr<Operator> GroupByAgg() {
  auto base = NationSupplierPartsupp();

  // group by
  auto ps_partkey = ColRefE(base, "ps_partkey");

  // aggregate
  auto value = Sum(
      Mul(ColRef(base, "ps_supplycost"), Float(ColRef(base, "ps_availqty"))));

  OperatorSchema schema;
  schema.AddDerivedColumn("ps_partkey", VirtColRef(ps_partkey, 0));
  schema.AddDerivedColumn("value", VirtColRef(value, 1));
  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(ps_partkey)),
      util::MakeVector(std::move(value)));
}

// subquery JOIN query
std::unique_ptr<Operator> JoinQuerySubquery() {
  auto subquery = SubqueryAgg();
  auto query = GroupByAgg();

  OperatorSchema schema;
  schema.AddPassthroughColumns(*subquery, 0);
  schema.AddPassthroughColumns(*query, 1);
  return std::make_unique<CrossProductOperator>(
      std::move(schema), std::move(subquery), std::move(query));
}

// Select(subquery JOIN query ON value > value1)
std::unique_ptr<Operator> SelectBase() {
  auto base = JoinQuerySubquery();
  auto gt = Gt(ColRef(base, "value"), ColRef(base, "value1"));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*base, {"ps_partkey", "value"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(base),
                                          std::move(gt));
}

// Order By revenue desc
std::unique_ptr<Operator> OrderBy() {
  auto agg = SelectBase();

  auto value = ColRef(agg, "value");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*agg);

  return std::make_unique<OrderByOperator>(std::move(schema), std::move(agg),
                                           util::MakeVector(std::move(value)),
                                           std::vector<bool>{false});
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(OrderBy());

  BenchVerify(*query, "benchmark/jcch1/raw/q11.tbl");
  return 0;
}
