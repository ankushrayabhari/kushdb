#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
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
using namespace kush::compile::cpp;
using namespace kush::catalog;
using namespace std::literals;

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
  auto eq = Eq(ColRef(region, "r_name"), Literal("MIDDLE EAST"sv));

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
      util::MakeVector(std::move(r_regionkey)),
      util::MakeVector(std::move(n_regionkey)));
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
      util::MakeVector(std::move(n_nationkey)),
      util::MakeVector(std::move(s_nationkey)));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"],
                             {"p_partkey", "p_mfgr", "p_size", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), "part");
}

// Select(p_size = 38 and p_type ENDS WITH 'TIN')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> ends_with =
        EndsWith(ColRef(part, "p_type"), Literal("TIN"sv));
    std::unique_ptr<Expression> eq = Eq(ColRef(part, "p_size"), Literal(38));
    cond = And(util::MakeVector(std::move(eq), std::move(ends_with)));
  }

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_partkey", "p_mfgr"});

  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(cond));
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
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(ps_partkey)));
}

// region_nation_supplier JOIN part_partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> RegionNationSupplierPartPartsupp() {
  auto rns = RegionNationSupplier();
  auto pps = PartPartsupp();

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
std::unique_ptr<Operator> SubqueryScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "nation");
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> SubqueryRegionNation() {
  auto region = SelectRegion();
  auto nation = SubqueryScanNation();

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
std::unique_ptr<Operator> SubqueryScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"], {"s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "supplier");
}

// region_nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> SubqueryRegionNationSupplier() {
  auto region_nation = SubqueryRegionNation();
  auto supplier = SubqueryScanSupplier();

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
std::unique_ptr<Operator> SubqueryScanPartsupp() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["partsupp"],
                             {"ps_suppkey", "ps_supplycost", "ps_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "partsupp");
}

// region_nation_supplier JOIN partsupp ON s_suppkey = ps_suppkey
std::unique_ptr<Operator> SubqueryRegionNationSupplierPartsupp() {
  auto rns = SubqueryRegionNationSupplier();
  auto partsupp = SubqueryScanPartsupp();

  auto s_suppkey = ColRef(rns, "s_suppkey", 0);
  auto ps_suppkey = ColRef(partsupp, "ps_suppkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*partsupp, {"ps_supplycost", "ps_partkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(rns), std::move(partsupp),
      util::MakeVector(std::move(s_suppkey)),
      util::MakeVector(std::move(ps_suppkey)));
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

// rnspps JOIN subquery ON ps_supplycost = min_ps_supplycost
std::unique_ptr<Operator> RNSPPSJoinSubquery() {
  auto rnspps = RegionNationSupplierPartPartsupp();
  auto subquery = Subquery();

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
std::unique_ptr<Operator> OrderBy() {
  auto base = RNSPPSJoinSubquery();

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

int main() {
  auto query = std::make_unique<OutputOperator>(OrderBy());

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
