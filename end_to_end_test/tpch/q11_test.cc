#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/parameters.h"
#include "end_to_end_test/schema.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "plan/operator/cross_product_operator.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

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

// Select(n_name = 'GERMANY')
std::unique_ptr<Operator> SelectNation() {
  auto nation = ScanNation();
  auto eq = Eq(ColRef(nation, "n_name"), Literal("GERMANY"sv));

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

// nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> NationSupplier() {
  auto nation = SelectNation();
  auto supplier = ScanSupplier();

  auto n_nationkey = ColRef(nation, "n_nationkey", 0);
  auto s_nationkey = ColRef(supplier, "s_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*supplier, {"s_suppkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation), std::move(supplier),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// Scan(partsupp)
std::unique_ptr<Operator> SubqueryScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_supplycost", "ps_availqty", "ps_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// nation_supplier JOIN partsupp ON ps_suppkey = s_suppkey
std::unique_ptr<Operator> SubqueryNationSupplierPartsupp() {
  auto nation_supplier = NationSupplier();
  auto partsupp = SubqueryScanPartsupp();

  auto s_suppkey = ColRef(nation_supplier, "s_suppkey", 0);
  auto ps_suppkey = ColRef(partsupp, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_availqty"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_supplier), std::move(partsupp),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
}

// Agg
std::unique_ptr<Operator> SubqueryAgg() {
  auto base = SubqueryNationSupplierPartsupp();

  // aggregate
  auto value = Sum(
      Mul(ColRef(base, "ps_supplycost"), Float(ColRef(base, "ps_availqty"))));

  OperatorSchema schema;
  schema.AddDerivedColumn("value1", Mul(VirtColRef(value, 0), Literal(0.0001)));

  return std::make_unique<AggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(value)));
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"], {"ps_partkey", "ps_supplycost",
                                              "ps_availqty", "ps_suppkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// nation_supplier JOIN partsupp ON ps_suppkey = s_suppkey
std::unique_ptr<Operator> NationSupplierPartsupp() {
  auto nation_supplier = NationSupplier();
  auto partsupp = ScanPartsupp();

  auto s_suppkey = ColRef(nation_supplier, "s_suppkey", 0);
  auto ps_suppkey = ColRef(partsupp, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *partsupp, {"ps_partkey", "ps_supplycost", "ps_availqty"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(nation_supplier), std::move(partsupp),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
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
      std::move(schema),
      util::MakeVector(std::move(subquery), std::move(query)));
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

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q11) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  auto expected_file = "end_to_end_test/tpch/q11_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_TRUE(
      CHECK_EQ_TBL(expected, output, query->Child().Schema().Columns()));
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "stack_spill"}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "linear_scan"}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, TPCHTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));

INSTANTIATE_TEST_SUITE_P(Adaptive, TPCHTest,
                         testing::Values(ParameterValues{
                             .pipeline_mode = "adaptive"}));
