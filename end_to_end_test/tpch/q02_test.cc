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
#include "end_to_end_test/test_util.h"
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
using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = Schema();

// Scan(region)
std::unique_ptr<Operator> ScanRegion(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion(const Database& db) {
  auto region = ScanRegion(db);
  auto eq = Eq(ColRef(region, "r_name"), Literal("MIDDLE EAST"sv));

  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_name", "n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> RegionNation(const Database& db) {
  auto region = SelectRegion(db);
  auto nation = ScanNation(db);

  auto r_regionkey = ColRef(region, "r_regionkey", 0);
  auto n_regionkey = ColRef(nation, "n_regionkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name", "n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      util::MakeVector(std::move(r_regionkey)),
      util::MakeVector(std::move(n_regionkey)));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"],
                             {"s_acctbal", "s_name", "s_address", "s_phone",
                              "s_comment", "s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// region_nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> RegionNationSupplier(const Database& db) {
  auto region_nation = RegionNation(db);
  auto supplier = ScanSupplier(db);

  auto n_nationkey = ColRef(region_nation, "n_nationkey", 0);
  auto s_nationkey = ColRef(supplier, "s_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region_nation, {"n_name"}, 0);
  schema.AddPassthroughColumns(
      *supplier,
      {"s_acctbal", "s_name", "s_address", "s_phone", "s_comment", "s_suppkey"},
      1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation), std::move(supplier),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"],
                             {"p_partkey", "p_mfgr", "p_size", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), db["part"]);
}

// Select(p_size = 35 and p_type ENDS WITH 'TIN')
std::unique_ptr<Operator> SelectPart(const Database& db) {
  auto part = ScanPart(db);

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> ends_with =
        EndsWith(ColRef(part, "p_type"), Literal("TIN"sv));
    std::unique_ptr<Expression> eq = Eq(ColRef(part, "p_size"), Literal(35));
    cond = And(util::MakeVector(std::move(eq), std::move(ends_with)));
  }

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"});

  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(cond));
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_partkey", "ps_suppkey", "ps_supplycost"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// part JOIN partsupp ON p_partkey = ps_partkey
std::unique_ptr<Operator> PartPartsupp(const Database& db) {
  auto part = SelectPart(db);
  auto partsupp = ScanPartsupp(db);

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto ps_partkey = ColRef(partsupp, "ps_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"}, 0);
  schema.AddPassthroughColumns(*partsupp, {"ps_suppkey", "ps_supplycost"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(partsupp),
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(ps_partkey)));
}

// region_nation_supplier JOIN part_partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> RegionNationSupplierPartPartsupp(const Database& db) {
  auto rns = RegionNationSupplier(db);
  auto pps = PartPartsupp(db);

  auto s_suppkey = ColRef(rns, "s_suppkey", 0);
  auto ps_suppkey = ColRef(pps, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *rns,
      {"s_acctbal", "s_name", "n_name", "s_address", "s_phone", "s_comment"},
      0);
  schema.AddPassthroughColumns(*pps, {"p_partkey", "p_mfgr", "ps_supplycost"},
                               1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rns), std::move(pps),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
}

// Scan(nation)
std::unique_ptr<Operator> SubqueryScanNation(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> SubqueryRegionNation(const Database& db) {
  auto region = SelectRegion(db);
  auto nation = SubqueryScanNation(db);

  auto r_regionkey = ColRef(region, "r_regionkey", 0);
  auto n_regionkey = ColRef(nation, "n_regionkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      util::MakeVector(std::move(r_regionkey)),
      util::MakeVector(std::move(n_regionkey)));
}

// Scan(supplier)
std::unique_ptr<Operator> SubqueryScanSupplier(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// region_nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> SubqueryRegionNationSupplier(const Database& db) {
  auto region_nation = SubqueryRegionNation(db);
  auto supplier = SubqueryScanSupplier(db);

  auto n_nationkey = ColRef(region_nation, "n_nationkey", 0);
  auto s_nationkey = ColRef(supplier, "s_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*supplier, {"s_suppkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation), std::move(supplier),
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// Scan(partsupp)
std::unique_ptr<Operator> SubqueryScanPartsupp(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_suppkey", "ps_supplycost", "ps_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// region_nation_supplier JOIN partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> SubqueryRegionNationSupplierPartsupp(
    const Database& db) {
  auto rns = SubqueryRegionNationSupplier(db);
  auto partsupp = SubqueryScanPartsupp(db);

  auto s_suppkey = ColRef(rns, "s_suppkey", 0);
  auto ps_suppkey = ColRef(partsupp, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_partkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rns), std::move(partsupp),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
}

std::unique_ptr<Operator> Subquery(const Database& db) {
  auto rnsps = SubqueryRegionNationSupplierPartsupp(db);
  auto ps_partkey = ColRefE(rnsps, "ps_partkey");
  auto min_supplycost = Min(ColRef(rnsps, "ps_supplycost"));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("ps_partkey", VirtColRef(ps_partkey, 0));
  schema.AddDerivedColumn("min_ps_supplycost", VirtColRef(min_supplycost, 1));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(rnsps),
      util::MakeVector(std::move(ps_partkey)),
      util::MakeVector(std::move(min_supplycost)));
}

// rnspps JOIN subquery ON ps_supplycost = min_ps_supplycost
std::unique_ptr<Operator> RNSPPSJoinSubquery(const Database& db) {
  auto rnspps = RegionNationSupplierPartPartsupp(db);
  auto subquery = Subquery(db);

  auto p_partkey = ColRef(rnspps, "p_partkey", 0);
  auto ps_supplycost = ColRef(rnspps, "ps_supplycost", 0);
  auto ps_partkey = ColRef(subquery, "ps_partkey", 1);
  auto min_ps_supplycost = ColRef(subquery, "min_ps_supplycost", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*rnspps,
                               {"s_acctbal", "s_name", "n_name", "p_partkey",
                                "p_mfgr", "s_address", "s_phone", "s_comment"},
                               0);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rnspps), std::move(subquery),
      util::MakeVector(std::move(p_partkey), std::move(ps_supplycost)),
      util::MakeVector(std::move(ps_partkey), std::move(min_ps_supplycost)));
}

// Order By s_acctbal desc, n_name, s_name, p_partkey
std::unique_ptr<Operator> OrderBy(const Database& db) {
  auto base = RNSPPSJoinSubquery(db);

  auto s_acctbal = ColRef(base, "s_acctbal");
  auto n_name = ColRef(base, "n_name");
  auto s_name = ColRef(base, "s_name");
  auto p_partkey = ColRef(base, "p_partkey");

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *base, {"s_acctbal", "s_name", "n_name", "p_partkey", "p_mfgr",
              "s_address", "s_phone", "s_comment"});

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(base),
      util::MakeVector(std::move(s_acctbal), std::move(n_name),
                       std::move(s_name), std::move(p_partkey)),
      std::vector<bool>{false, true, true, true});
}

class TPCHTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(TPCHTest, Q02) {
  SetFlags(GetParam());

  auto db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy(db));

  auto expected_file = "end_to_end_test/tpch/q02_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  EXPECT_EQ_TBL(expected, output, query->Child().Schema().Columns(), 1e-5);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "stack_spill"}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, TPCHTest,
                         testing::Values(ParameterValues{
                             .backend = "asm", .reg_alloc = "stack_spill"}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, TPCHTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));
