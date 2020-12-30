#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/string_comparison_expression.h"
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
using namespace kush::compile::cpp;
using namespace kush::catalog;

const Database db = Schema();

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});
  return std::make_unique<ScanOperator>(std::move(schema), "region");
}

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion() {
  auto region = ScanRegion();
  auto eq = Eq(ColRef(region, "r_name"), Literal("MIDDLE EAST"));

  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_name", "n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "nation");
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> RegionNation() {
  auto region = SelectRegion();
  auto nation = ScanNation();

  auto r_regionkey = ColRef(region, "r_regionkey", 0);
  auto n_regionkey = ColRef(nation, "n_regionkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name", "n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      std::move(r_regionkey), std::move(n_regionkey));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"],
                             {"s_acctbal", "s_name", "s_address", "s_phone",
                              "s_comment", "s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "supplier");
}

// region_nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> RegionNationSupplier() {
  auto region_nation = RegionNation();
  auto supplier = ScanSupplier();

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
      std::move(n_nationkey), std::move(s_nationkey));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"],
                             {"p_partkey", "p_mfgr", "p_size", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), "part");
}

// Select(p_size = 15 and p_type ENDS WITH 'TIN')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  auto eq = And({Eq(ColRef(part, "p_size"), Literal(15)),
                 EndsWith(ColRef(part, "p_type"), Literal("TIN"))});

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"});

  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(eq));
}

// Scan(partsupp)
std::unique_ptr<Operator> ScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_partkey", "ps_suppkey", "ps_supplycost"});
  return std::make_unique<ScanOperator>(std::move(schema), "partsupp");
}

// part JOIN partsupp ON p_partkey = ps_partkey
std::unique_ptr<Operator> PartPartsupp() {
  auto part = SelectPart();
  auto partsupp = ScanPartsupp();

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto ps_partkey = ColRef(partsupp, "ps_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"}, 0);
  schema.AddPassthroughColumns(*partsupp, {"ps_suppkey", "ps_supplycost"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(partsupp),
      std::move(p_partkey), std::move(ps_partkey));
}

// region_nation_supplier JOIN part_partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> RegionNationSupplierPartPartsupp() {
  auto rns = RegionNationSupplier();
  auto pps = PartPartsupp();

  auto s_suppkey = ColRef(rns, "s_suppkey", 0);
  auto ps_suppkey = ColRef(pps, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*rns,
                               {"s_acctbal", "s_name", "n_name", "s_address",
                                "s_phone", "s_comment", "s_suppkey"},
                               0);
  schema.AddPassthroughColumns(*pps, {"p_partkey", "p_mfgr", "ps_supplycost"},
                               1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rns), std::move(pps), std::move(s_suppkey),
      std::move(ps_suppkey));
}

int main() {
  auto query = std::make_unique<OutputOperator>(OrderBy());

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
