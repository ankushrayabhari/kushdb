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
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/scan_select_operator.h"
#include "plan/operator/select_operator.h"
#include "plan/operator/simd_scan_select_operator.h"
#include "util/builder.h"
#include "util/time_execute.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

Database db;

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});

  auto eq =
      Exp(Eq(VirtColRef(scan_schema, "r_name"), Literal("MIDDLE EAST"sv)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"r_regionkey"});
  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["region"],
      util::MakeVector(std::move(eq)));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_name", "n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"],
                             {"s_acctbal", "s_name", "s_address", "s_phone",
                              "s_comment", "s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Select(p_size = 1 and p_type ENDS WITH 'MINED GOLD')
std::unique_ptr<Operator> SelectPart() {
  OperatorSchema scan_schema;
  scan_schema.AddGeneratedColumns(db["part"],
                                  {"p_partkey", "p_mfgr", "p_size", "p_type"});

  auto ends_with =
      Exp(EndsWith(VirtColRef(scan_schema, "p_type"), Literal("MINED GOLD"sv)));
  auto eq = Exp(Eq(VirtColRef(scan_schema, "p_size"), Literal(1)));

  OperatorSchema schema;
  schema.AddVirtualPassthroughColumns(scan_schema, {"p_partkey", "p_mfgr"});

  return std::make_unique<ScanSelectOperator>(
      std::move(schema), std::move(scan_schema), db["part"],
      util::MakeVector(std::move(eq), std::move(ends_with)));
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_partkey", "ps_suppkey", "ps_supplycost"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// Scan(nation)
std::unique_ptr<Operator> SubqueryScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

// Scan(supplier)
std::unique_ptr<Operator> SubqueryScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
}

// Scan(partsupp)
std::unique_ptr<Operator> SubqueryScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_suppkey", "ps_supplycost", "ps_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["partsupp"]);
}

// region
// JOIN nation ON r_regionkey = n_regionkey
// JOIN supplier ON n_nationkey = s_nationkey
// JOIN partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> SubqueryRegionNationSupplierPartsupp() {
  auto region = SelectRegion();
  auto nation = SubqueryScanNation();
  auto supplier = SubqueryScanSupplier();
  auto partsupp = SubqueryScanPartsupp();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(region, "r_regionkey", 0), ColRef(nation, "n_regionkey", 1)));
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 1), ColRef(supplier, "s_nationkey", 2)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 2), ColRef(partsupp, "ps_suppkey", 3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_partkey"}, 3);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(region), std::move(nation),
                       std::move(supplier), std::move(partsupp)),
      std::move(conditions));
}

std::unique_ptr<Operator> Subquery() {
  auto rnsps = SubqueryRegionNationSupplierPartsupp();
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

// region
// JOIN nation ON r_regionkey = n_regionkey
// JOIN supplier ON n_nationkey = s_nationkey
// JOIN partsupp ON s_suppkey = ps_suppkey
// JOIN part ON p_partkey = ps_partkey
// JOIN subquery ON ps_supplycost = min_ps_supplycost AND p_partkey = ps_partkey
std::unique_ptr<Operator> RegionNationSupplierPartPartsuppSubquery() {
  auto region = SelectRegion();
  auto nation = ScanNation();
  auto supplier = ScanSupplier();
  auto partsupp = ScanPartsupp();
  auto part = SelectPart();
  auto subquery = Subquery();

  std::vector<std::unique_ptr<Expression>> conditions;
  conditions.push_back(
      Eq(ColRef(region, "r_regionkey", 0), ColRef(nation, "n_regionkey", 1)));
  conditions.push_back(
      Eq(ColRef(nation, "n_nationkey", 1), ColRef(supplier, "s_nationkey", 2)));
  conditions.push_back(
      Eq(ColRef(supplier, "s_suppkey", 2), ColRef(partsupp, "ps_suppkey", 3)));
  conditions.push_back(
      Eq(ColRef(partsupp, "ps_partkey", 3), ColRef(part, "p_partkey", 4)));
  conditions.push_back(
      Eq(ColRef(part, "p_partkey", 4), ColRef(subquery, "ps_partkey", 5)));
  conditions.push_back(Eq(ColRef(partsupp, "ps_supplycost", 3),
                          ColRef(subquery, "min_ps_supplycost", 5)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name"}, 1);
  schema.AddPassthroughColumns(
      *supplier, {"s_acctbal", "s_name", "s_address", "s_phone", "s_comment"},
      2);
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"}, 4);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema),
      util::MakeVector(std::move(region), std::move(nation),
                       std::move(supplier), std::move(partsupp),
                       std::move(part), std::move(subquery)),
      std::move(conditions));
}

// Order By s_acctbal desc, n_name, s_name, p_partkey
std::unique_ptr<Operator> OrderBy() {
  auto base = RegionNationSupplierPartPartsuppSubquery();

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

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  db = Schema();
  auto query = std::make_unique<OutputOperator>(OrderBy());

  TimeExecute(*query);
  return 0;
}
